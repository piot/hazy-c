/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <hazy/latency.h>
#include <math.h>

void hazyLatencyInit(HazyLatency* self, HazyLatencyConfig config, Clog log)
{
    self->log = log;
    self->latency = (HazyLatencyMs) (config.minLatency + config.maxLatency) / 2;
    self->targetLatency = self->latency;
    self->config = config;
    self->phase = HazyLatencyPhaseNormal;
    self->nextDriftEstimationMs = 0;
    self->lastUpdateTimeMs = 0;
}

void hazyLatencySetConfig(HazyLatency* self, HazyLatencyConfig config)
{
    self->latency = (HazyLatencyMs) (config.minLatency + config.maxLatency) / 2;
    self->targetLatency = self->latency;
    self->config = config;
    self->phase = HazyLatencyPhaseNormal;
    self->nextDriftEstimationMs = 0;
}

static bool reachTargetLatency(HazyLatency* self, float deltaSeconds)
{
    int diff = ((int) self->targetLatency - (int) self->precisionLatency);
    if (abs(diff) < 1) {
        //CLOG_C_VERBOSE(&self->log, "hit target %f", self->precisionLatency)
        self->latency = self->targetLatency;
        return true;
    }

    float changeThisTick = self->latencyDiffPerSecond * deltaSeconds;
    float floatDiff = (float) diff;
    float floatAbsDiff = fabsf(floatDiff);
    if (changeThisTick >= floatAbsDiff) {
        changeThisTick = floatAbsDiff;
    }

    //CLOG_C_VERBOSE(&self->log, "deltaTime: %f changeThisTick: %f", deltaSeconds, changeThisTick)

    self->precisionLatency += copysignf(changeThisTick, floatDiff);
    self->latency = (HazyLatencyMs) self->precisionLatency;

    return false;
}

int hazyLatencyGetLatencyWithJitter(HazyLatency* self)
{
    HazyLatencyMs jitterForThisPacket = (HazyLatencyMs) rand() % (self->config.latencyJitter * 2 + 1);

    bool jitterSpikeThisPacket = (rand() % (int)self->config.chanseJitterSpike) == 0;
    if (jitterSpikeThisPacket)
    {
        jitterForThisPacket *= 3;
    }

    return self->latency + jitterForThisPacket;
}

static HazyLatencyMs calculateTargetLatency(HazyLatency* self)
{
    size_t diff = self->config.maxLatency - self->config.minLatency;
    if (diff == 0) {
        diff = 1;
    }
    return (HazyLatencyMs) self->config.minLatency + (HazyLatencyMs) rand() % diff;
}

static float calculateLatencyChangePerSecond(HazyLatency* self)
{
    const float normalRamp = 2.0f;
    const float aggressiveRamp = 50.0f;

    bool timeForAggressiveRamp = (rand() % 10) == 0;
    if (timeForAggressiveRamp) {
#if defined CLOG_LOG_ENABLED
        CLOG_C_VERBOSE(&self->log, "time for aggressive ramp")
#else
        (void) self;
#endif
    }

    return timeForAggressiveRamp ? aggressiveRamp : normalRamp;
}

void hazyLatencyUpdate(HazyLatency* self, MonotonicTimeMs now)
{
    if (!self->lastUpdateTimeMs) {
        self->lastUpdateTimeMs = now;
    }
    MonotonicTimeMs deltaMs = now - self->lastUpdateTimeMs;
    self->lastUpdateTimeMs = now;

    switch (self->phase) {
        case HazyLatencyPhaseNormal:
            if (now >= self->nextDriftEstimationMs) {
                self->phase = HazyLatencyPhaseDrifting;
                self->targetLatency = calculateTargetLatency(self);
                self->latencyDiffPerSecond = calculateLatencyChangePerSecond(self);
                self->precisionLatency = self->latency;
                self->nextDriftEstimationMs = 0;
                CLOG_C_VERBOSE(&self->log, "new target latency: %d", self->targetLatency)
            }

            break;
        case HazyLatencyPhaseDrifting: {
            float deltaSeconds = (float) deltaMs / 1000.0f;
            bool reachedTarget = reachTargetLatency(self, deltaSeconds);
            if (reachedTarget) {
                CLOG_C_VERBOSE(&self->log, "drifting complete %d", self->latency)
                self->phase = HazyLatencyPhaseNormal;
                MonotonicTimeMs nextTime = rand() % 1000 + 200;
                self->nextDriftEstimationMs = now + nextTime;
            }
        } break;
    }
#if defined HAZY_LOG_ENABLE
    CLOG_C_VERBOSE(&self->log, "latency: %d (phase %d)", self->latency, self->phase)
#endif
}

HazyLatencyConfig hazyLatencyGoodCondition(void)
{
    HazyLatencyConfig config = {38/2, 45/2, 6, 1000};

    return config;
}

HazyLatencyConfig hazyLatencyRecommended(void)
{
    HazyLatencyConfig config = {70/2, 90/2, 16, 50};

    return config;
}

HazyLatencyConfig hazyLatencyWorstCase(void)
{
    HazyLatencyConfig config = {100, 180, 32, 30};

    return config;
}
