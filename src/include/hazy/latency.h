/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef HAZY_LATENCY_H
#define HAZY_LATENCY_H

#include <hazy/decider.h>
#include <clog/clog.h>
#include <discoid/circular_buffer.h>
#include <hazy/packets.h>
#include <monotonic-time/monotonic_time.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct HazyLatencyConfig {
    size_t minLatency;
    size_t maxLatency;
    size_t latencyJitter;
} HazyLatencyConfig;

typedef uint16_t HazyLatencyMs;

typedef struct HazyLatency {
    HazyLatencyMs latency;
    HazyLatencyMs targetLatency;
    MonotonicTimeMs lastDriftEstimationMs;
    HazyLatencyConfig config;
    Clog log;
} HazyLatency;

void hazyLatencyInit(HazyLatency* self, HazyLatencyConfig config, Clog log);
void hazyLatencySetConfig(HazyLatency* self, HazyLatencyConfig config);
void hazyLatencyUpdate(HazyLatency* self, MonotonicTimeMs now);
int hazyLatencyGetLatencyWithJitter(HazyLatency* self);

HazyLatencyConfig hazyLatencyGoodCondition(void);
HazyLatencyConfig hazyLatencyRecommended(void);
HazyLatencyConfig hazyLatencyWorstCase(void);

#endif

