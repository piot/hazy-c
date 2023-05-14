/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <hazy/latency.h>
#include <math.h>

void hazyLatencyInit(HazyLatency* self, HazyLatencyConfig config, Clog log)
{
    self->log = log;
    self->latency = (config.minLatency +  config.maxLatency) / 2;
    self->targetLatency = self->latency;
    self->config = config;
    self->phase = HazyLatencyPhaseNormal;
    self->nextDriftEstimationMs = 0;
    self->lastUpdateTimeMs = 0;
}

void hazyLatencySetConfig(HazyLatency* self, HazyLatencyConfig config)
{
    self->latency = (config.minLatency +  config.maxLatency) / 2;
    self->targetLatency = self->latency;
    self->config = config;
    self->phase = HazyLatencyPhaseNormal;
    self->nextDriftEstimationMs = 0;
}

static bool reachTargetLatency(HazyLatency* self, float deltaSeconds)
{
    int diff = ((int) self->targetLatency - (int) self->precisionLatency);
    if (abs(diff) < 2) {
        return true;
    }

    float changeThisTick = 0.5f * deltaSeconds;
    if (changeThisTick >= abs(diff)) {
        return true;
    }
    self->precisionLatency += changeThisTick * tc_sign(diff);
    self->latency = (int) self->precisionLatency;

    return false;
}

int hazyLatencyGetLatencyWithJitter(HazyLatency* self)
{
    int jitterForThisPacket = rand() % self->config.latencyJitter;
    return self->latency + jitterForThisPacket;
}

static HazyLatencyMs calculateTargetLatency(HazyLatency* self)
{
    int diff = self->config.maxLatency - self->config.minLatency;
    if (diff == 0) {
        diff = 1;
    }
    return self->config.minLatency + rand() % diff;
}

void hazyLatencyUpdate(HazyLatency* self, MonotonicTimeMs now)
{
    if (!self->lastUpdateTimeMs) {
        self->lastUpdateTimeMs = now;
    }
    int deltaMs = now - self->lastUpdateTimeMs;
    self->lastUpdateTimeMs = now;

    switch (self->phase) {
        case HazyLatencyPhaseNormal:
            if (now >= self->nextDriftEstimationMs) {
                self->phase = HazyLatencyPhaseDrifting;
                self->targetLatency = calculateTargetLatency(self);
                self->precisionLatency = self->latency;
                self->nextDriftEstimationMs = 0;
                CLOG_C_VERBOSE(&self->log, "new target latency: %d", self->targetLatency)
            }
            break;
        case HazyLatencyPhaseDrifting: {
            float deltaSeconds = deltaMs / 1000.0f;
            bool reachedTarget = reachTargetLatency(self, deltaSeconds);
            if (reachedTarget) {
                CLOG_C_VERBOSE(&self->log, "drifting complete %d", self->latency)
                self->phase = HazyLatencyPhaseNormal;
                MonotonicTimeMs nextTime = rand() % 1000 + 200;
                self->nextDriftEstimationMs = now + nextTime;
            }
        } break;
    }
#if HAZY_LOG_ENABLE
    CLOG_C_VERBOSE(&self->log, "latency: %d (phase %d)", self->latency, self->phase)
#endif
}

HazyLatencyConfig hazyLatencyGoodCondition(void)
{
    HazyLatencyConfig config = {11, 70, 3};

    return config;
}

HazyLatencyConfig hazyLatencyRecommended(void)
{
    HazyLatencyConfig config = {82, 180, 6};

    return config;
}

HazyLatencyConfig hazyLatencyWorstCase(void)
{
    HazyLatencyConfig config = {170, 220, 31};

    return config;
}
