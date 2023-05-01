/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef HAZY_DECISION_H
#define HAZY_DECISION_H

#include <stddef.h>
#include <clog/clog.h>

typedef enum HazyDecision {
    HazyDecisionDuplicate,
    HazyDecisionDrop,
    HazyDecisionGarble,
    HazyDecisionOriginal,
    HazyDecisionReorder,
    HazyDecisionMAX
} HazyDecision;

typedef struct HazyDecisionRange {
    size_t max;
    HazyDecision decision;
} HazyDecisionRange;

typedef struct HazyDecider {
    size_t max;
    size_t rangeCount;
    HazyDecision decision;
    HazyDecisionRange ranges[10];
    Clog log;
} HazyDecider;

typedef struct HazyDeciderConfig {
    size_t originalChance;
    size_t dropChance;
    size_t duplicateChance;
} HazyDeciderConfig;

HazyDeciderConfig hazyDeciderGoodCondition(void);
HazyDeciderConfig hazyDeciderRecommended(void);
HazyDeciderConfig hazyDeciderWorstCase(void);

void hazyDeciderInit(HazyDecider* rangeCollection, HazyDeciderConfig config, Clog log);
HazyDecision hazyDeciderDecide(HazyDecider* self);
void hazyDeciderSetConfig(HazyDecider* self, HazyDeciderConfig config);

#endif
