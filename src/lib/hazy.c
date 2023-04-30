/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <hazy/hazy.h>
#include <stdbool.h>
#include <udp-transport/udp_transport.h>

#define HAZY_LOG_ENABLE (0)

static void hazyPacketSet(HazyPacket* packet, const uint8_t* buf, size_t octetsRead, MonotonicTimeMs timeToAct)
{
    packet->octetCount = octetsRead;
    packet->data = tc_malloc(octetsRead);
    tc_memcpy_octets((void*) packet->data, buf, octetsRead);
    packet->timeToAct = timeToAct;
    packet->created = monotonicTimeMsNow();
}

static bool hazyIsItTime(MonotonicTimeMs query)
{
    MonotonicTimeMs now = monotonicTimeMsNow();
    return now >= query;
}

static HazyPacket* hazyFindFreePacket(HazyPackets* self)
{
    for (size_t i = 0; i < self->capacity; ++i) {
        struct HazyPacket* packet = &self->packets[i];
        if (!packet->octetCount) {
            return packet;
        }
    }

    return 0;
}

static const HazyPacket* hazyFindPacketToActOn(const HazyPackets* self)
{
    MonotonicTimeMs now = monotonicTimeMsNow();
    MonotonicTimeMs minimumToActOn = 0;
    const HazyPacket* foundPacket = 0;
    for (size_t i = 0; i < self->capacity; ++i) {
        const HazyPacket* packet = &self->packets[i];
        if (packet->octetCount && now >= packet->timeToAct) {
            MonotonicTimeMs timeSinceThreshold = packet->timeToAct - now;
            if (timeSinceThreshold < minimumToActOn) {
                minimumToActOn = timeSinceThreshold;
                foundPacket = packet;
            }
        }
    }

    if (foundPacket) {
#if HAZY_LOG_ENABLE
        MonotonicTimeMs nowMs = monotonicTimeMsNow();
        MonotonicTimeMs delayedMs = nowMs - foundPacket->created;
        MonotonicTimeMs intendedLatencyMs = nowMs - foundPacket->timeToAct;
        CLOG_C_VERBOSE(&self->log, "found packet: actual latency: %ld (intended was %ld) time: %lu", delayedMs,
                       intendedLatencyMs, nowMs)
        if (delayedMs > 100) {
            delayedMs = 0;
        }
#endif
    }

    return foundPacket;
}

static void calculateRanges(HazyRanges* rangeCollection, HazyConfigDirection config)
{
    int last = 0;
    int index = 0;

    HazyRange* ranges = rangeCollection->ranges;

    if (config.originalChance > 0) {
        last += config.originalChance;
        ranges[index].max = last;
        ranges[index].decision = HazyDecisionOriginal;
        index++;
    }

    if (config.dropChance > 0) {
        last += config.dropChance;
        ranges[index].max = last;
        ranges[index].decision = HazyDecisionDrop;
        index++;
    }

    if (config.duplicateChance > 0) {
        last += config.duplicateChance;
        ranges[index].max = last;
        ranges[index].decision = HazyDecisionDuplicate;
        index++;
    }

    rangeCollection->max = last;
    rangeCollection->rangeCount = index;
}

static void hazyPacketsInit(HazyPackets* packets)
{
    packets->capacity = HAZY_PACKETS_CAPACITY;
    // tc_mem_clear_type_n(packets->packets, packets->capacity);
    for (size_t i = 0; i < packets->capacity; ++i) {
        packets->packets[i].indexForDebug = i;
        packets->packets[i].octetCount = 0;
        packets->packets[i].data = 0;
        packets->packets[i].timeToAct = 0;
    }
    packets->lastAddedIndex = 0;
    packets->lastAddedIndexIsValid = false;
}

static void hazyDirectionInit(HazyDirection* self, size_t capacity, HazyConfigDirection config)
{
    hazyPacketsInit(&self->packets);
    calculateRanges(&self->ranges, config);

    self->latency = config.maxLatency;
}

static void hazyDirectionReset(HazyDirection* self)
{
    hazyPacketsInit(&self->packets);
}

void hazyInit(Hazy* self, size_t capacity, HazyConfig config, Clog log)
{
    tc_snprintf(self->out.debugPrefix, 32, "%s/hazy/out", log.constantPrefix);
    self->out.log.config = log.config;
    self->out.log.constantPrefix = self->out.debugPrefix;
    hazyDirectionInit(&self->out, capacity, config.out);

    tc_snprintf(self->in.debugPrefix, 32, "%s/hazy/in", log.constantPrefix);
    self->in.log.config = log.config;
    self->in.log.constantPrefix = self->in.debugPrefix;
    hazyDirectionInit(&self->in, capacity, config.in);

    self->log = log;
}

void hazyReset(Hazy* self)
{
    hazyDirectionReset(&self->out);
    hazyDirectionReset(&self->in);
}

void hazySetConfig(Hazy* self, HazyConfig config)
{
    calculateRanges(&self->in.ranges, config.in);
    self->in.latency = config.in.maxLatency;
    calculateRanges(&self->out.ranges, config.out);
    self->out.latency = config.in.maxLatency;
}

/// decide
/// @param self
/// @return
static HazyDecision hazyDeciderDecide(HazyRanges* self)
{
    size_t value = rand() % self->max;

    for (size_t i = 0; i < self->rangeCount; ++i) {
        if (self->ranges[i].max > value) {
            return self->ranges[i].decision;
        }
    }

    CLOG_ERROR("hazy: internal error")
}

static int hazyWriteInternal(HazyDirection* self, const uint8_t* data, size_t octetCount, MonotonicTimeMs proposedTime)
{
    if (octetCount == 0) {
        return 0;
    }

    HazyPacket* packet = hazyFindFreePacket(&self->packets);
    if (packet == 0) {
        CLOG_C_WARN(&self->log, "couldn't find free packet. dropping a packet", self->log)
        return -2;
    }
    self->packets.lastAddedIndex = packet->indexForDebug;
    self->packets.lastAddedIndexIsValid = true;

    hazyPacketSet(packet, data, octetCount, proposedTime);

#if HAZY_LOG_ENABLE
    CLOG_C_VERBOSE(&self->log, "packet set index %d,  %zu, latency: %lu", packet->indexForDebug, packet->octetCount,
                   randomMillisecondsLatency);
#endif

    return 0;
}

static void hazyDestroyPacket(HazyPacket* self)
{
    tc_free((void*) self->data);
    self->data = 0;
    self->octetCount = 0;
}

static int hazySend(HazyPackets* self, UdpTransportInOut* socket, Clog* log)
{
    while (1) {
        const HazyPacket* packet = hazyFindPacketToActOn(self);
        if (packet == 0) {
            break;
        }

        CLOG_C_VERBOSE(log, "send index:%zu %zu ms %zu", packet->indexForDebug, packet->timeToAct, packet->octetCount)
        int errorCode = udpTransportSend(socket, packet->data, packet->octetCount);
        if (errorCode < 0) {
            return errorCode;
        }
        hazyDestroyPacket((HazyPacket*) packet);
    }

    return 0;
}

static int hazyPacketsFeed(HazyPackets* self, const uint8_t* buf, size_t octetsRead, MonotonicTimeMs proposedTime,
                           Clog* log)
{
    HazyPacket* packet = hazyFindFreePacket(self);
    if (packet == 0) {
        CLOG_C_WARN(log, "out of capacity, dropping one packet");
        return -3;
    }

    self->lastAddedIndex = packet->indexForDebug;

    hazyPacketSet(packet, buf, octetsRead, proposedTime);

#if HAZY_LOG_ENABLE

    CLOG_C_VERBOSE(log, "feed packet %d octetCount: %zu, latency: %lu time:%lu", packet->indexForDebug,
                   packet->octetCount, randomMillisecondsLatency, now);
#endif

    return 0;
}

static int hazyWriteOut(HazyDirection* self, const uint8_t* data, size_t octetCount, bool reorderAllowed)
{
#if HAZY_LOG_ENABLE
    CLOG_C_VERBOSE(&self->log, "write out %zu octetCount latency:%d", octetCount, self->latency);
#endif

    MonotonicTimeMs now = monotonicTimeMsNow();
    MonotonicTimeMs proposedTime = now + self->latency;

    if (self->packets.lastAddedIndexIsValid) {
        MonotonicTimeMs lastPacketMs = self->packets.packets[self->packets.lastAddedIndex].timeToAct;
        if ((proposedTime <= lastPacketMs) && !reorderAllowed) {
            proposedTime = lastPacketMs + 1;
        }
    }

    return hazyWriteInternal(self, data, octetCount, proposedTime);
}

static int hazyWriteDirection(HazyDirection* self, const uint8_t* data, size_t octetCount)
{
    HazyDecision decision = hazyDeciderDecide(&self->ranges);

    int result = 0;
    switch (decision) {
        case HazyDecisionDrop:
            CLOG_C_VERBOSE(&self->log, "dropped packet")
            return 0;
        case HazyDecisionDuplicate:
            CLOG_C_VERBOSE(&self->log, "duplicate packet")
            hazyWriteOut(self, data, octetCount, false);
            result = hazyWriteOut(self, data, octetCount, false);
            break;
        case HazyDecisionReorder: {
            CLOG_C_VERBOSE(&self->log, "reorder packet")
            MonotonicTimeMs latency = self->latency;
            self->latency = -12;
            result = hazyWriteOut(self, data, octetCount, true);
            self->latency = latency;
        } break;
        case HazyDecisionGarble: {
            uint8_t temp[1200];
            for (size_t index = 0; index < octetCount; ++index) {
                temp[index] = (uint8_t) rand();
            }
            result = hazyWriteOut(self, temp, octetCount, false);
            CLOG_C_VERBOSE(&self->log, "garble packet")
        } break;
        case HazyDecisionOriginal:
            CLOG_C_VERBOSE(&self->log, "original")
            result = hazyWriteOut(self, data, octetCount, false);
            break;
        case HazyDecisionMAX:
            break;
    }

    return result;
}

int hazyWrite(Hazy* self, const uint8_t* data, size_t octetLength)
{
    return hazyWriteDirection(&self->out, data, octetLength);
}

static int hazyReadFromUdp(HazyDirection* self, UdpTransportInOut* socket)
{
    static uint8_t buf[1200];
    int octetsRead = udpTransportReceive(socket, buf, 1200);
    if (octetsRead <= 0) {
        return octetsRead;
    }

#if HAZY_LOG_ENABLE || 1
    CLOG_C_VERBOSE(&self->log, "read from udp %d", octetsRead);
#endif

    return hazyWriteDirection(self, buf, octetsRead);
}

static int sign(int x)
{
    return (x > 0) ? 1 : ((x < 0) ? -1 : 0);
}

static MonotonicTimeMs updateLatency(HazyDirection* self)
{
    int diff = (self->targetLatency - self->latency);
    if (diff == 0) {
        diff = 1;
    }
    int delta = rand() % abs(diff);
    delta += rand() % 40;
    delta *= sign(diff);
    if (((int) self->latency + delta) > 0) {
        self->latency += delta;
    }
#if HAZY_LOG_ENABLE || 1
    CLOG_C_VERBOSE(&self->log, "new latency: %d", self->latency)
#endif

    return self->latency;
}

void hazyUpdate(Hazy* self)
{
    // updateLatency(self);
#if HAZY_LOG_ENABLE
    MonotonicTimeMs now = monotonicTimeMsNow();
    CLOG_C_VERBOSE(&self->log, "time is now %lu", now);
#endif
}

int hazyUpdateAndCommunicate(Hazy* self, UdpTransportInOut* socket)
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
    return hazyPacketsFeed(&self->in.packets, data, capacity, self->in.latency, &self->in.log);
}

static int hazyPacketsRead(HazyPackets* self, uint8_t* data, size_t capacity, Clog* log)
{
    const HazyPacket* packet = hazyFindPacketToActOn(self);
    if (packet == 0) {
        return 0;
    }
    size_t octetCount = packet->octetCount;
    if (octetCount <= capacity) {
        tc_memcpy_octets(data, packet->data, octetCount);
#if HAZY_LOG_ENABLE
        CLOG_C_VERBOSE(log, "read copied to target. index: %d, octetCount: %zu", packet->indexForDebug,
                       packet->octetCount);
#endif
    } else {
        CLOG_C_WARN(log, "couldn't copy to target, capacity too small")
        octetCount = -4;
    }

    hazyDestroyPacket((HazyPacket*) packet);

    return octetCount;
}

int hazyRead(Hazy* self, uint8_t* data, size_t capacity)
{
    return hazyPacketsRead(&self->in.packets, data, capacity, &self->log);
}

int hazyReadSend(Hazy* self, uint8_t* data, size_t capacity)
{
    return hazyPacketsRead(&self->out.packets, data, capacity, &self->log);
}

HazyConfigDirection hazyConfigDirectionRecommended(void)
{
    HazyConfigDirection config = {100, 1, 3, 14, 150, 4};

    return config;
}

HazyConfigDirection hazyConfigDirectionWorstCase(void)
{
    HazyConfigDirection config = {100, 10, 10, 34, 150, 20};

    return config;
}

HazyConfig hazyConfigRecommended(void)
{
    HazyConfig config = {hazyConfigDirectionRecommended(), hazyConfigDirectionRecommended()};
    return config;
}

HazyConfig hazyConfigWorstCase(void)
{
    HazyConfig config = {hazyConfigDirectionRecommended(), hazyConfigDirectionRecommended()};
    return config;
}
