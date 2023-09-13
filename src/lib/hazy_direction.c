/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <hazy/direction.h>

static HazyLatencyConfig halfConfig(HazyLatencyConfig config)
{
    config.latencyJitter = config.latencyJitter;
    config.minLatency = config.minLatency / 2;
    config.maxLatency = config.maxLatency / 2;

    return config;
}

void hazyDirectionInit(HazyDirection* self, size_t capacity, struct ImprintAllocatorWithFree* allocatorWithFree,
                       HazyDirectionConfig config, Clog log)
{
    (void) capacity;

    self->nextPacketDropBurstMs = 0;
    self->nextPacketDropBurstEndMs = 0;
    hazyPacketsInit(&self->packets, allocatorWithFree);
    hazyDeciderInit(&self->decider, config.decider, log);
    hazyLatencyInit(&self->latency, halfConfig(config.latency), log);
    self->config = config.direction;
    self->phase = HazyDirectionPhaseNormal;
}

void hazyDirectionReset(HazyDirection* self)
{
    hazyPacketsInit(&self->packets, self->packets.allocatorWithFree);
}

void hazyDirectionSetConfig(HazyDirection* self, HazyDirectionConfig config)
{
    hazyDeciderSetConfig(&self->decider, config.decider);
    hazyLatencySetConfig(&self->latency, halfConfig(config.latency));
    self->config = config.direction;
}

static int hazyWriteInternal(HazyDirection* self, const uint8_t* data, size_t octetCount, MonotonicTimeMs proposedTime)
{
    if (octetCount == 0) {
        return 0;
    }

    if (self->packets.packetCount >= HAZY_PACKETS_CAPACITY) {
        CLOG_C_VERBOSE(&self->log, "overflow out of packet capacity %d", HAZY_PACKETS_CAPACITY)
        return -45;
    }

    HazyPacket* packet = hazyPacketsWrite(&self->packets, data, octetCount, proposedTime, &self->log);

#if defined HAZY_LOG_ENABLE
    CLOG_C_VERBOSE(&self->log, "packet set index %d,  %zu, latency: %lu", packet->indexForDebug, packet->octetCount,
                   randomMillisecondsLatency)
#else
    (void) packet;

#endif

    return 0;
}

static int hazyWriteOut(HazyDirection* self, const uint8_t* data, size_t octetCount, bool reorderAllowed)
{
#if defined HAZY_LOG_ENABLE
    CLOG_C_VERBOSE(&self->log, "write out %zu octetCount latency:%d", octetCount, self->latency)
#endif

    MonotonicTimeMs now = monotonicTimeMsNow();
    MonotonicTimeMs proposedTime = now + hazyLatencyGetLatencyWithJitter(&self->latency);

    if (self->packets.lastTimeIsValid) {
        if ((proposedTime <= self->packets.lastTimeAdded) && !reorderAllowed) {
            proposedTime = self->packets.lastTimeAdded + 1;
        }
    }

    return hazyWriteInternal(self, data, octetCount, proposedTime);
}

void hazyDirectionUpdate(HazyDirection* self, MonotonicTimeMs now)
{
    switch (self->phase) {
        case HazyDirectionPhaseNormal:
            if (now >= self->nextPacketDropBurstMs && self->config.dropBurstTimeSpanMs != 0) {
                size_t dropDuration = (size_t) (rand() % (int) self->config.dropBurstTimeSpanMs) +
                                      self->config.dropBurstTimeMinimumMs;
                CLOG_C_DEBUG(&self->log, "start packet drop burst for %zu ms", dropDuration)
                self->phase = HazyDirectionPhasePacketDropBurst;
                self->nextPacketDropBurstEndMs = now + (MonotonicTimeMs) dropDuration;
                self->nextPacketDropBurstMs = 0;
            }
            break;
        case HazyDirectionPhasePacketDropBurst:
            if (now >= self->nextPacketDropBurstEndMs && self->config.timeBetweenDropBurstSpanMs) {
                size_t timeUntilNextDropBurst = (size_t) (rand() % (int) self->config.timeBetweenDropBurstSpanMs) +
                                                self->config.timeBetweenDropBurstMinimumMs;
                CLOG_C_DEBUG(&self->log, "packet drop burst over. Will wait %zu ms until the next one", timeUntilNextDropBurst)
                self->phase = HazyDirectionPhaseNormal;
                self->nextPacketDropBurstMs = now + (MonotonicTimeMs) timeUntilNextDropBurst;
            }
            break;
    }
}

int hazyWriteDirection(HazyDirection* self, const uint8_t* data, size_t octetCount)
{
    if (self->phase == HazyDirectionPhasePacketDropBurst) {
        CLOG_C_VERBOSE(&self->log, "decision: dropped packet due to packet drop burst")
        return 0;
    }

    HazyDecision decision = hazyDeciderDecide(&self->decider);

    int result = 0;
    switch (decision) {
        case HazyDecisionDrop:
            CLOG_C_VERBOSE(&self->log, "decision: dropped packet")
            return 0;
        case HazyDecisionDuplicate: {
            int count = (rand() % 3) + 1;
            CLOG_C_VERBOSE(&self->log, "decision: duplicate packet %d count", count)
            for (int i = 0; i < count; ++i) {
                hazyWriteOut(self, data, octetCount, false);
                result = hazyWriteOut(self, data, octetCount, false);
            }
        } break;
        case HazyDecisionOutOfOrder: {
            CLOG_C_VERBOSE(&self->log, "decision: out of order packet")
            // Send this in the future so it is likely reordered
            HazyLatencyMs latency = self->latency.latency;
            const int sendInterval = 16; // ms
            const int reorderLatency = sendInterval * ((rand() % 3) + 1);
            self->latency.latency += reorderLatency;
            result = hazyWriteOut(self, data, octetCount, true);
            self->latency.latency = latency;
        } break;
        case HazyDecisionTamper: {
            uint8_t temp[1200];
            for (size_t index = 0; index < octetCount; ++index) {
                temp[index] = (uint8_t) rand();
            }
            result = hazyWriteOut(self, temp, octetCount, false);
            CLOG_C_VERBOSE(&self->log, "decision: garble packet")
        } break;
        case HazyDecisionOriginal:
            CLOG_C_VERBOSE(&self->log, "decision: original")
            result = hazyWriteOut(self, data, octetCount, false);
            break;
    }

    return result;
}

HazyDirectionOnlyConfig hazyDirectionOnlyConfigGoodCondition(void)
{
    HazyDirectionOnlyConfig config = {0, 0, 0, 0};
    return config;
}

HazyDirectionOnlyConfig hazyDirectionOnlyConfigRecommended(void)
{
    HazyDirectionOnlyConfig config = {60000, 10000, 100, 10};
    return config;
}

HazyDirectionOnlyConfig hazyDirectionOnlyConfigWorstCase(void)
{
    HazyDirectionOnlyConfig config = {10000, 5000, 100, 50};
    return config;
}

HazyDirectionConfig hazyDirectionConfigGoodCondition(void)
{
    HazyDirectionConfig config = {hazyDeciderGoodCondition(), hazyLatencyGoodCondition(), hazyDirectionOnlyConfigGoodCondition()};
    return config;
}

HazyDirectionConfig hazyDirectionConfigRecommended(void)
{
    HazyDirectionConfig config = {hazyDeciderRecommended(), hazyLatencyRecommended(), hazyDirectionOnlyConfigRecommended()};
    return config;
}

HazyDirectionConfig hazyDirectionConfigWorstCase(void)
{
    HazyDirectionConfig config = {hazyDeciderWorstCase(), hazyLatencyWorstCase(), hazyDirectionOnlyConfigWorstCase()};
    return config;
}
