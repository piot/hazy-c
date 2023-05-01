/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <hazy/transport.h>

static int hazyDatagramTransportSendFn(void* self_, const uint8_t* data, size_t size)
{
    HazyDatagramTransportInOut* self = self_;

    return hazyWrite(&self->hazy, data, size);
}

static int hazyDatagramTransportReceiveFn(void* self_, uint8_t* data, size_t size)
{
    HazyDatagramTransportInOut* self = self_;

    return hazyRead(&self->hazy, data, size);
}

void hazyDatagramTransportInOutInit(HazyDatagramTransportInOut* self, UdpTransportInOut other,
                                    struct ImprintAllocator* allocator,
                                    struct ImprintAllocatorWithFree* allocatorWithFree, HazyConfig config, Clog log)
{
    self->transport.receive = hazyDatagramTransportReceiveFn;
    self->transport.send = hazyDatagramTransportSendFn;
    self->transport.self = self;

    self->other = other;

    hazyInit(&self->hazy, 60, allocator, allocatorWithFree, config, log);
}

void hazyDatagramTransportInOutUpdate(HazyDatagramTransportInOut* self)
{
    hazyUpdateAndCommunicate(&self->hazy, &self->other);
}
