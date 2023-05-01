/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <hazy/decider.h>
#include <hazy/hazy.h>

static void recalculateRanges(HazyDecider* self, HazyDeciderConfig config)
{
    int last = 0;
    int index = 0;

    HazyDecisionRange* ranges = self->ranges;

    if (config.originalChance > 0) {
        last += config.originalChance;
        ranges[index].max = last;
        ranges[index].decision = HazyDecisionOriginal;
        index++;
    }

    if (config.dropChance > 0) {
        last += config.dropChance;
        ranges[index].max = last;
        ranges[index].decision = HazyDecisionDrop;
        index++;
    }

    if (config.duplicateChance > 0) {
        last += config.duplicateChance;
        ranges[index].max = last;
        ranges[index].decision = HazyDecisionDuplicate;
        index++;
    }

    self->max = last;
    self->rangeCount = index;
}

void hazyDeciderInit(HazyDecider* self, HazyDeciderConfig config, Clog log)
{
    self->log = log;
    recalculateRanges(self, config);
}

void hazyDeciderSetConfig(HazyDecider* self, HazyDeciderConfig config)
{
    recalculateRanges(self, config);
}

/// decide
/// @param self
/// @return
HazyDecision hazyDeciderDecide(HazyDecider* self)
{
    size_t value = rand() % self->max;

    for (size_t i = 0; i < self->rangeCount; ++i) {
        if (self->ranges[i].max > value) {
            //CLOG_C_VERBOSE(&self->log, "decision was: %d", self->ranges[i].decision)
            return self->ranges[i].decision;
        }
    }

    CLOG_ERROR("hazy: internal error")
}

HazyDeciderConfig hazyDeciderGoodCondition(void)
{
    HazyDeciderConfig config = {100, 1, 3, 14, 25, 2};

    return config;
}

HazyDeciderConfig hazyDeciderRecommended(void)
{
    HazyDeciderConfig config = {100, 1, 7, 14, 150, 4};

    return config;
}

HazyDeciderConfig hazyDeciderWorstCase(void)
{
    HazyDeciderConfig config = {100, 10, 10, 34, 250, 20};

    return config;
}
