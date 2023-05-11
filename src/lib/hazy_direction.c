/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <hazy/direction.h>

static HazyLatencyConfig halfConfig(HazyLatencyConfig config)
{
    config.latencyJitter = config.latencyJitter / 2;
    config.minLatency = config.minLatency / 2;
    config.maxLatency = config.maxLatency / 2;

    return config;
}

void hazyDirectionInit(HazyDirection* self, size_t capacity, struct ImprintAllocatorWithFree* allocatorWithFree,
                       HazyDirectionConfig config, Clog log)
{
    hazyPacketsInit(&self->packets, allocatorWithFree);
    hazyDeciderInit(&self->decider, config.decider, log);
    hazyLatencyInit(&self->latency, halfConfig(config.latency), log);
}

void hazyDirectionReset(HazyDirection* self)
{
    hazyPacketsInit(&self->packets, self->packets.allocatorWithFree);
}

void hazyDirectionSetConfig(HazyDirection* self, HazyDirectionConfig config)
{
    hazyDeciderSetConfig(&self->decider, config.decider);
    hazyLatencySetConfig(&self->latency, halfConfig(config.latency));
}

static int hazyWriteInternal(HazyDirection* self, const uint8_t* data, size_t octetCount, MonotonicTimeMs proposedTime)
{
    if (octetCount == 0) {
        return 0;
    }

    if (self->packets.packetCount >= HAZY_PACKETS_CAPACITY) {
        CLOG_C_ERROR(&self->log, "overflow out of space")
        return -45;
    }

    HazyPacket* packet = hazyPacketsWrite(&self->packets, data, octetCount, proposedTime, &self->log);

#if HAZY_LOG_ENABLE
    CLOG_C_VERBOSE(&self->log, "packet set index %d,  %zu, latency: %lu", packet->indexForDebug, packet->octetCount,
                   randomMillisecondsLatency);
#endif

    return 0;
}

static int hazyWriteOut(HazyDirection* self, const uint8_t* data, size_t octetCount, bool reorderAllowed)
{
#if HAZY_LOG_ENABLE
    CLOG_C_VERBOSE(&self->log, "write out %zu octetCount latency:%d", octetCount, self->latency);
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

int hazyWriteDirection(HazyDirection* self, const uint8_t* data, size_t octetCount)
{
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
            MonotonicTimeMs latency = self->latency.latency;
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

HazyDirectionConfig hazyDirectionConfigGoodCondition(void)
{
    HazyDirectionConfig config = {hazyDeciderGoodCondition(), hazyLatencyGoodCondition()};
    return config;
}

HazyDirectionConfig hazyDirectionConfigRecommended(void)
{
    HazyDirectionConfig config = {hazyDeciderRecommended(), hazyLatencyRecommended()};
    return config;
}

HazyDirectionConfig hazyDirectionConfigWorstCase(void)
{
    HazyDirectionConfig config = {hazyDeciderWorstCase(), hazyLatencyWorstCase()};
    return config;
}
