/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef HAZY_TRANSPORT_H
#define HAZY_TRANSPORT_H

#include <hazy/hazy.h>
#include <udp-transport/udp_transport.h>

typedef struct HazyDatagramTransportInOut {
    UdpTransportInOut transport;
    Hazy hazy;
    UdpTransportInOut other;
} HazyDatagramTransportInOut;

void hazyDatagramTransportInOutInit(HazyDatagramTransportInOut* self, UdpTransportInOut other, HazyConfig config, Clog log);
void hazyDatagramTransportInOutUpdate(HazyDatagramTransportInOut* self);

#endif
