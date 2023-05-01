/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef HAZY_DECISION_H
#define HAZY_DECISION_H

#include <clog/clog.h>
#include <stddef.h>

typedef enum HazyDecision {
    HazyDecisionDuplicate,
    HazyDecisionDrop,
    HazyDecisionTamper,
    HazyDecisionOriginal,
    HazyDecisionOutOfOrder,
} HazyDecision;

typedef struct HazyDecisionRange {
    size_t max;
    HazyDecision decision;
} HazyDecisionRange;

typedef struct HazyDecider {
    size_t max;
    size_t rangeCount;
    HazyDecision decision;
    HazyDecisionRange ranges[5]; // NOTE: Must match number of fields in HazyDeciderConfig
    Clog log;
} HazyDecider;

typedef struct HazyDeciderConfig {
    size_t originalChance;
    size_t dropChance;
    size_t outOfOrderChance;
    size_t duplicateChance;
    size_t tamperChance;
} HazyDeciderConfig;

HazyDeciderConfig hazyDeciderGoodCondition(void);
HazyDeciderConfig hazyDeciderRecommended(void);
HazyDeciderConfig hazyDeciderWorstCase(void);

void hazyDeciderInit(HazyDecider* rangeCollection, HazyDeciderConfig config, Clog log);
HazyDecision hazyDeciderDecide(HazyDecider* self);
void hazyDeciderSetConfig(HazyDecider* self, HazyDeciderConfig config);

#endif
