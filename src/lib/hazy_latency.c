/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <hazy/latency.h>

void hazyLatencyInit(HazyLatency* self, HazyLatencyConfig config, Clog log)
{
    self->log = log;
    self->latency = config.maxLatency;
    self->targetLatency = self->latency;
    self->config = config;
}

void hazyLatencySetConfig(HazyLatency* self, HazyLatencyConfig config)
{
    self->latency = config.maxLatency;
    self->targetLatency = self->latency;
    self->config = config;
}

static void reachTargetLatency(HazyLatency* self)
{
    int diff = ((int)self->targetLatency - (int)self->latency);
    if (diff == 0) {
        diff = 1;
    }
    int delta = rand() % abs(diff);
    delta += rand() % 2;
    delta *= tc_sign(diff);
    if (((int) self->latency + delta) > 0) {
        self->latency += delta;
    }
}

int hazyLatencyGetLatencyWithJitter(HazyLatency* self)
{
    int jitterForThisPacket = rand() % self->config.latencyJitter;
    //CLOG_C_VERBOSE(&self->log, "jitter: %d", jitterForThisPacket);
    return self->latency + jitterForThisPacket;
}

void hazyLatencyUpdate(HazyLatency* self, MonotonicTimeMs now)
{
    reachTargetLatency(self);

#if HAZY_LOG_ENABLE && 0
    CLOG_C_VERBOSE(&self->log, "new latency: %d", self->latency)
#endif
}

HazyLatencyConfig hazyLatencyGoodCondition(void)
{
    HazyLatencyConfig config = {11, 44, 2};

    return config;
}

HazyLatencyConfig hazyLatencyRecommended(void)
{
    HazyLatencyConfig config = {33, 72, 6};

    return config;
}

HazyLatencyConfig hazyLatencyWorstCase(void)
{
    HazyLatencyConfig config = {130, 180, 31};

    return config;
}
