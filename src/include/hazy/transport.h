/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef HAZY_TRANSPORT_H
#define HAZY_TRANSPORT_H

#include <hazy/hazy.h>
#include <datagram-transport/transport.h>

struct ImprintAllocatorWithFree;
struct ImprintAllocator;

typedef struct HazyDatagramTransportInOut {
    DatagramTransport transport;
    Hazy hazy;
    DatagramTransport other;
} HazyDatagramTransportInOut;

void hazyDatagramTransportInOutInit(HazyDatagramTransportInOut* self, DatagramTransport other,
                                    struct ImprintAllocator* allocator,
                                    struct ImprintAllocatorWithFree* allocatorWithFree, HazyConfig config, Clog log);
void hazyDatagramTransportInOutUpdate(HazyDatagramTransportInOut* self);

#endif
