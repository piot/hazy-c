/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef HAZY_DIRECTION_H
#define HAZY_DIRECTION_H

#include <clog/clog.h>
#include <discoid/circular_buffer.h>
#include <hazy/latency.h>
#include <hazy/packets.h>
#include <monotonic-time/monotonic_time.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct ImprintAllocatorWithFree;
struct ImprintAllocator;

typedef struct HazyDirectionOnlyConfig {
    size_t timeBetweenDropBurstSpanMs;
    size_t timeBetweenDropBurstMinimumMs;
    size_t dropBurstTimeSpanMs;
    size_t dropBurstTimeMinimumMs;
} HazyDirectionOnlyConfig;

typedef struct HazyDirectionConfig {
    HazyDeciderConfig decider;
    HazyLatencyConfig latency;
    HazyDirectionOnlyConfig direction;
} HazyDirectionConfig;

typedef enum HazyDirectionPhase {
    HazyDirectionPhaseNormal,
    HazyDirectionPhasePacketDropBurst,
} HazyDirectionPhase;

typedef struct HazyDirection {
    HazyDirectionPhase phase;
    HazyPackets packets;
    HazyLatency latency;
    char debugPrefix[32];
    HazyDecider decider;
    MonotonicTimeMs nextPacketDropBurstMs;
    MonotonicTimeMs nextPacketDropBurstEndMs;
    HazyDirectionOnlyConfig config;
    Clog log;
} HazyDirection;

void hazyDirectionInit(HazyDirection* self, size_t capacity, struct ImprintAllocatorWithFree* allocatorWithFree,
                       HazyDirectionConfig config, Clog log);
void hazyDirectionReset(HazyDirection* self);
void hazyDirectionSetConfig(HazyDirection* self, HazyDirectionConfig config);
int hazyWriteDirection(HazyDirection* self, const uint8_t* data, size_t octetCount);
void hazyDirectionUpdate(HazyDirection* self, MonotonicTimeMs now);

HazyDirectionConfig hazyDirectionConfigGoodCondition(void);
HazyDirectionConfig hazyDirectionConfigRecommended(void);
HazyDirectionConfig hazyDirectionConfigWorstCase(void);

HazyDirectionOnlyConfig hazyDirectionOnlyConfigGoodCondition(void);
HazyDirectionOnlyConfig hazyDirectionOnlyConfigRecommended(void);
HazyDirectionOnlyConfig hazyDirectionOnlyConfigWorstCase(void);

#endif
