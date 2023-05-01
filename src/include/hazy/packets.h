/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef HAZY_PACKETS_H
#define HAZY_PACKETS_H

#include <clog/clog.h>
#include <discoid/circular_buffer.h>
#include <monotonic-time/monotonic_time.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct HazyPacket {
    const uint8_t* data;
    size_t octetCount;
    MonotonicTimeMs timeToAct;
    int indexForDebug;
    MonotonicTimeMs created;
} HazyPacket;

#define HAZY_PACKETS_CAPACITY (256)

typedef struct HazyPackets {
    HazyPacket packets[HAZY_PACKETS_CAPACITY];
    size_t packetCount;
    size_t capacity;
    struct ImprintAllocatorWithFree* allocatorWithFree;
    MonotonicTimeMs lastTimeAdded;
    bool lastTimeIsValid;
} HazyPackets;

void hazyPacketsInit(HazyPackets* self, struct ImprintAllocatorWithFree* allocator);
int hazyPacketsRead(HazyPackets* self, uint8_t* data, size_t capacity, Clog* log);
HazyPacket* hazyPacketsWrite(HazyPackets* self, const uint8_t* buf, size_t octetsRead, MonotonicTimeMs timeToAct,
                             Clog* log);

void hazyPacketsDestroyPacket(HazyPackets* self, HazyPacket* packetToDiscard);

const HazyPacket* hazyPacketsFindPacketToActOn(const HazyPackets* self);

#endif
