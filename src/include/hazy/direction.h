/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef HAZY_DIRECTION_H
#define HAZY_DIRECTION_H

#include <clog/clog.h>
#include <discoid/circular_buffer.h>
#include <hazy/packets.h>
#include <hazy/latency.h>
#include <monotonic-time/monotonic_time.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct ImprintAllocatorWithFree;
struct ImprintAllocator;

typedef struct HazyDirectionConfig {
    HazyDeciderConfig decider;
    HazyLatencyConfig latency;
} HazyDirectionConfig;


typedef struct HazyDirection {
    HazyPackets packets;
    HazyLatency latency;
    char debugPrefix[32];
    HazyDecider decider;
    Clog log;
} HazyDirection;

void hazyDirectionInit(HazyDirection* self, size_t capacity, struct ImprintAllocatorWithFree* allocatorWithFree, HazyDirectionConfig config, Clog log);
void hazyDirectionReset(HazyDirection* self);
void hazyDirectionSetConfig(HazyDirection* self, HazyDirectionConfig config);
int hazyWriteDirection(HazyDirection* self, const uint8_t* data, size_t octetCount);

HazyDirectionConfig hazyDirectionConfigGoodCondition(void);
HazyDirectionConfig hazyDirectionConfigRecommended(void);
HazyDirectionConfig hazyDirectionConfigWorstCase(void);

#endif
