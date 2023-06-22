/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <hazy/hazy.h>
#include <imprint/allocator.h>
#include <datagram-transport/transport.h>
#include <inttypes.h>

#define HAZY_LOG_ENABLE (0)

void hazyInit(Hazy* self, size_t capacity, ImprintAllocator* allocator, ImprintAllocatorWithFree* allocatorWithFree,
              HazyConfig config, Clog log)
{
    tc_snprintf(self->out.debugPrefix, 32, "%s/hazy/out", log.constantPrefix);
    self->out.log.config = log.config;
    self->out.log.constantPrefix = self->out.debugPrefix;
    hazyDirectionInit(&self->out, capacity, allocatorWithFree, config.out, self->out.log);

    tc_snprintf(self->in.debugPrefix, 32, "%s/hazy/in", log.constantPrefix);
    self->in.log.config = log.config;
    self->in.log.constantPrefix = self->in.debugPrefix;
    hazyDirectionInit(&self->in, capacity, allocatorWithFree, config.in, self->in.log);

    discoidBufferInit(&self->receiveBuffer, allocator, 32 * 1024);

    self->log = log;
}

void hazyReset(Hazy* self)
{
    hazyDirectionReset(&self->out);
    hazyDirectionReset(&self->in);
}

void hazySetConfig(Hazy* self, HazyConfig config)
{
    hazyDirectionSetConfig(&self->in, config.in);
    hazyDirectionSetConfig(&self->out, config.out);
}

static int hazySend(HazyPackets* self, DatagramTransport* socket, Clog* log)
{
    while (1) {
        const HazyPacket* packet = hazyPacketsFindPacketToActOn(self);
        if (packet == 0) {
            break;
        }

        CLOG_C_VERBOSE(log, "send index:%d %" PRIX64 " ms %zu", packet->indexForDebug, packet->timeToAct, packet->octetCount)
        int errorCode = datagramTransportSend(socket, packet->data, packet->octetCount);
        if (errorCode < 0) {
            return errorCode;
        }
        hazyPacketsDestroyPacket(self, (HazyPacket*) packet);
    }

    return 0;
}

static int hazyPacketsFeed(HazyPackets* self, const uint8_t* buf, size_t octetsRead, MonotonicTimeMs proposedTime,
                           Clog* log)
{
    HazyPacket* packet = hazyPacketsWrite(self, buf, octetsRead, proposedTime, log);

#if HAZY_LOG_ENABLE

    CLOG_C_VERBOSE(log, "feed packet %d octetCount: %zu, latency: %lu time:%lu", packet->indexForDebug,
                   packet->octetCount, randomMillisecondsLatency, now);
#endif

    return 0;
}

int hazyWrite(Hazy* self, const uint8_t* data, size_t octetLength)
{
    return hazyWriteDirection(&self->out, data, octetLength);
}

static int hazyReadFromUdp(HazyDirection* self, DatagramTransport* socket)
{
    static uint8_t buf[1200];
    int octetsRead = datagramTransportReceive(socket, buf, 1200);
    if (octetsRead <= 0) {
        return octetsRead;
    }

#if HAZY_LOG_ENABLE || 1
    CLOG_C_VERBOSE(&self->log, "read from transport %d", octetsRead);
#endif

    return hazyWriteDirection(self, buf, octetsRead);
}

static void movePacketsToIncomingBuffer(Hazy* self)
{
    while (1) {
        const HazyPacket* packet = hazyPacketsFindPacketToActOn(&self->in.packets);
        if (packet == 0) {
            return;
        }
        DiscoidBuffer* receiveBuffer = &self->receiveBuffer;

        const size_t headerSize = 2;
        size_t neededOctetCount = packet->octetCount + headerSize;
        if (discoidBufferWriteAvailable(receiveBuffer) < neededOctetCount) {
            CLOG_C_NOTICE(&self->log, "receive buffer is full, so intentionally dropping package")
        } else {
            uint16_t serializeOctetCount = packet->octetCount;
            //            int64_t monotonicTime = packet->timeToAct;
            discoidBufferWrite(receiveBuffer, (const uint8_t*) &serializeOctetCount, 2);
            discoidBufferWrite(receiveBuffer, packet->data, packet->octetCount);
        }

        hazyPacketsDestroyPacket(&self->in.packets, (HazyPacket*) packet);
    }
}

void hazyUpdate(Hazy* self)
{
    MonotonicTimeMs now = monotonicTimeMsNow();
    hazyLatencyUpdate(&self->in.latency, now);
    hazyLatencyUpdate(&self->out.latency, now);

    movePacketsToIncomingBuffer(self);
}

int hazyUpdateAndCommunicate(Hazy* self, DatagramTransport* socket)
{
    hazyUpdate(self);
    hazySend(&self->out.packets, socket, &self->out.log);

    for (size_t i = 0; i < 30; ++i) {
        int result = hazyReadFromUdp(&self->in, socket);
        if (result < 0) {
            return result;
        }
    }

    return 0;
}

int hazyFeedRead(Hazy* self, const uint8_t* data, size_t capacity)
{
    return hazyPacketsFeed(&self->in.packets, data, capacity, self->in.latency.latency, &self->in.log);
}

int hazyRead(Hazy* self, uint8_t* data, size_t capacity)
{
    if (discoidBufferReadAvailable(&self->receiveBuffer) < 2) {
        return 0;
    }

    uint16_t octetLength = 0;
    int error = discoidBufferRead(&self->receiveBuffer, (uint8_t*) &octetLength, sizeof(uint16_t));
    if (error < 0) {
        return error;
    }

    if (capacity < octetLength) {
        CLOG_C_ERROR(&self->log, "packet length greater than capacity")
    }

    discoidBufferRead(&self->receiveBuffer, data, octetLength);

    return (int) octetLength;
}

int hazyReadSend(Hazy* self, uint8_t* data, size_t capacity)
{
    return hazyPacketsRead(&self->out.packets, data, capacity, &self->log);
}

HazyConfig hazyConfigGoodCondition(void)
{
    HazyConfig config = {hazyDirectionConfigGoodCondition(), hazyDirectionConfigGoodCondition()};
    return config;
}

HazyConfig hazyConfigRecommended(void)
{
    HazyConfig config = {hazyDirectionConfigRecommended(), hazyDirectionConfigRecommended()};
    return config;
}

HazyConfig hazyConfigWorstCase(void)
{
    HazyConfig config = {hazyDirectionConfigWorstCase(), hazyDirectionConfigWorstCase()};
    return config;
}
