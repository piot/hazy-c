/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <hazy/hazy.h>
#include <imprint/allocator.h>
#include <stdbool.h>

#define HAZY_LOG_ENABLE (0)

void hazyPacketsInit(HazyPackets* self, struct ImprintAllocatorWithFree* allocator)
{
    self->allocatorWithFree = allocator;
    self->packetCount = 0;
    self->capacity = HAZY_PACKETS_CAPACITY;
    for (size_t i = 0; i < self->capacity; ++i) {
        self->packets[i].indexForDebug = (int) i;
        self->packets[i].octetCount = 0;
        self->packets[i].data = 0;
        self->packets[i].timeToAct = 0;
    }
    self->lastTimeAdded = 0;
    self->lastTimeIsValid = false;
}

static HazyPacket* hazyPacketsFindFree(HazyPackets* self)
{
    for (size_t i = 0; i < self->capacity; ++i) {
        if (self->packets[i].data == 0) {
            return &self->packets[i];
        }
    }

    return 0;
}

int hazyPacketsRead(HazyPackets* self, uint8_t* data, size_t capacity, Clog* log)
{
    HazyPacket* packet = hazyPacketsFindPacketToActOn(self);
    if (packet == 0) {
        return 0;
    }
    size_t octetCount = packet->octetCount;
    int returnValue = (int) octetCount;
    if (octetCount <= capacity) {
        tc_memcpy_octets(data, packet->data, octetCount);
#if defined HAZY_LOG_ENABLE
        CLOG_C_VERBOSE(log, "read copied to target. index: %d, octetCount: %zu", packet->indexForDebug,
                       octetCount);
#else
        (void) log;
        (void) packet;
#endif
    } else {
        (void) log;
        CLOG_C_WARN(log, "couldn't copy to target, capacity too small")
        returnValue = -4;
    }
    hazyPacketsDestroyPacket(self, packet);

    return returnValue;
}

HazyPacket* hazyPacketsWrite(HazyPackets* self, const uint8_t* buf, size_t octetsRead, MonotonicTimeMs timeToAct,
                             Clog* log)
{
    HazyPacket* packet = hazyPacketsFindFree(self);
    if (packet == 0) {
        CLOG_C_ERROR(log, "out of capacity")
    }

    packet->octetCount = octetsRead;
    uint8_t* target = IMPRINT_ALLOC_TYPE_COUNT(&self->allocatorWithFree->allocator, uint8_t, octetsRead);
    tc_memcpy_octets(target, buf, octetsRead);
    packet->data = target;
    packet->timeToAct = timeToAct;
    packet->created = monotonicTimeMsNow();
    self->packetCount++;
    self->lastTimeAdded = timeToAct;
    self->lastTimeIsValid = true;

    return packet;
}

void hazyPacketsDestroyPacket(HazyPackets* self, HazyPacket* packetToDiscard)
{
    if (packetToDiscard->data == 0) {
        CLOG_ERROR("illegal discard")
    }
    if (self->packetCount == 0) {
        CLOG_ERROR("internal error")
    }
    IMPRINT_FREE(self->allocatorWithFree, (void*) packetToDiscard->data);
    packetToDiscard->data = 0;
    packetToDiscard->octetCount = 0;
    self->packetCount--;
}

HazyPacket* hazyPacketsFindPacketToActOn(HazyPackets* self)
{
    MonotonicTimeMs now = monotonicTimeMsNow();
    MonotonicTimeMs minimumToActOn = 0;
    HazyPacket* foundPacket = 0;
    for (size_t i = 0; i < self->capacity; ++i) {
        HazyPacket* packet = &self->packets[i];
        if (!packet->octetCount) {
            continue;
        }
        if (now >= packet->timeToAct) {
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
