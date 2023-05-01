/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef HAZY_HAZY_H
#define HAZY_HAZY_H

#include "decider.h"
#include <clog/clog.h>
#include <discoid/circular_buffer.h>
#include <hazy/packets.h>
#include <hazy/latency.h>
#include <monotonic-time/monotonic_time.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <hazy/direction.h>

struct UdpTransportInOut;
struct ImprintAllocatorWithFree;
struct ImprintAllocator;

typedef struct HazyConfig {
    HazyDirectionConfig in;
    HazyDirectionConfig out;
} HazyConfig;

typedef struct Hazy {
    HazyDirection out;
    HazyDirection in;
    DiscoidBuffer receiveBuffer;
    Clog log;
} Hazy;

void hazyInit(Hazy* self, size_t capacity, struct ImprintAllocator* allocator, struct ImprintAllocatorWithFree* allocatorWithFree, HazyConfig config, Clog log);
void hazyReset(Hazy* self);
void hazyUpdate(Hazy* self);
int hazyUpdateAndCommunicate(Hazy* self, struct UdpTransportInOut* socket);
int hazyRead(Hazy* self, uint8_t* data, size_t capacity);
int hazyWrite(Hazy* self, const uint8_t* data, size_t octetCount);
void hazySetConfig(Hazy* self, HazyConfig config);
int hazyReadSend(Hazy* self, uint8_t* data, size_t capacity);
int hazyFeedRead(Hazy* self, const uint8_t* data, size_t capacity);

HazyConfig hazyConfigGoodCondition(void);
HazyConfig hazyConfigRecommended(void);
HazyConfig hazyConfigWorstCase(void);

#endif
