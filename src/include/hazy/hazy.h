/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef HAZY_HAZY_H
#define HAZY_HAZY_H

#include <clog/clog.h>
#include <monotonic-time/monotonic_time.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct UdpTransportInOut;

typedef enum HazyDecision {
    HazyDecisionDuplicate,
    HazyDecisionDrop,
    HazyDecisionGarble,
    HazyDecisionOriginal,
    HazyDecisionReorder,
    HazyDecisionMAX
} HazyDecision;

typedef struct HazyPacket {
    const uint8_t* data;
    size_t octetCount;
    MonotonicTimeMs timeToAct;
    int indexForDebug;
    MonotonicTimeMs created;
} HazyPacket;

#define HAZY_PACKETS_CAPACITY (256)

typedef struct HazyPackets {
    HazyPacket packets[HAZY_PACKETS_CAPACITY];
    size_t capacity;
    size_t lastAddedIndex;
    bool lastAddedIndexIsValid;
} HazyPackets;

typedef struct HazyRange {
    size_t max;
    HazyDecision decision;
} HazyRange;

typedef struct HazyRanges {
    size_t max;
    size_t rangeCount;
    HazyDecision decision;
    HazyRange ranges[10];
} HazyRanges;

typedef struct HazyDirection {
    HazyPackets packets;
    MonotonicTimeMs latency;
    MonotonicTimeMs targetLatency;
    char debugPrefix[32];
    HazyRanges ranges;
    Clog log;
} HazyDirection;

typedef struct HazyConfigDirection {
    size_t originalChance;
    size_t dropChance;
    size_t duplicateChance;
    size_t minLatency;
    size_t maxLatency;
    size_t latencyJitter;
} HazyConfigDirection;

typedef struct HazyConfig {
    HazyConfigDirection in;
    HazyConfigDirection out;
} HazyConfig;

typedef struct Hazy {
    HazyDirection out;
    HazyDirection in;
    Clog log;
} Hazy;

void hazyInit(Hazy* self, size_t capacity, HazyConfig config, Clog log);
void hazyReset(Hazy* self);
void hazyUpdate(Hazy* self);
int hazyUpdateAndCommunicate(Hazy* self, struct UdpTransportInOut* socket);
int hazyRead(Hazy* self, uint8_t* data, size_t capacity);
int hazyWrite(Hazy* self, const uint8_t* data, size_t octetCount);
void hazySetConfig(Hazy* self, HazyConfig config);
int hazyReadSend(Hazy* self, uint8_t* data, size_t capacity);
int hazyFeedRead(Hazy* self, const uint8_t* data, size_t capacity);

HazyConfigDirection hazyConfigDirectionRecommended(void);
HazyConfigDirection hazyConfigDirectionWorstCase(void);

HazyConfig hazyConfigRecommended(void);
HazyConfig hazyConfigWorstCase(void);

#endif
