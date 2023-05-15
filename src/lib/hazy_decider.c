/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <hazy/decider.h>
#include <hazy/hazy.h>

#define ADD_RANGE(fieldName, enumName)                                                                                 \
    if (config.fieldName > 0) {                                                                                        \
        last += config.fieldName;                                                                                      \
        ranges[index].max = last;                                                                                      \
        ranges[index].decision = enumName;                                                                             \
        index++;                                                                                                       \
    }

static void recalculateRanges(HazyDecider* self, HazyDeciderConfig config)
{
    int last = 0;
    int index = 0;

    HazyDecisionRange* ranges = self->ranges;

    ADD_RANGE(originalChance, HazyDecisionOriginal)
    ADD_RANGE(dropChance, HazyDecisionDrop)
    ADD_RANGE(outOfOrderChance, HazyDecisionOutOfOrder)
    ADD_RANGE(duplicateChance, HazyDecisionDuplicate)
    ADD_RANGE(tamperChance, HazyDecisionTamper)

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
        if (value < self->ranges[i].max) {
            return self->ranges[i].decision;
        }
    }

    CLOG_ERROR("hazy: internal error")
}

HazyDeciderConfig hazyDeciderGoodCondition(void)
{
    HazyDeciderConfig config = {100000, 1, 30, 30, 0};

    return config;
}

HazyDeciderConfig hazyDeciderRecommended(void)
{
    HazyDeciderConfig config = {10000, 30, 20, 10, 0};

    return config;
}

HazyDeciderConfig hazyDeciderWorstCase(void)
{
    HazyDeciderConfig config = {200, 5, 5, 5, 0};

    return config;
}
