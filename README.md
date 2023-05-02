# Hazy

> uncertain or confused

Very basic Internet simulator for unreliable datagram transports (i.e UDP/IP). Useful for testing that an application will work reliably on Internet conditions.

Can simulate:

* Duplicate packets
* Out of Order packets
* Tamper packets
* Drop Packets
* Latency drift
* Latency jitter

## Upcoming features

* Throttle
* Bandwidth usage warning

## Usage

### Initialize

Initialize a `HazyDatagramTransportInOut`.

Use `hazyConfigRecommended()` to get a default `HazyConfig`.
The `other` is the datagram transport that Hazy should 'wrap'.

The `transport` field in `HazyDatagramTransportInOut` should be used for sending and receiving datagrams.

```c
void hazyDatagramTransportInOutInit(HazyDatagramTransportInOut* self, UdpTransportInOut other,
                                    struct ImprintAllocator* allocator,
                                    struct ImprintAllocatorWithFree* allocatorWithFree, HazyConfig config, Clog log);
```

### Update

Call this in the application update loop:

```c
void hazyDatagramTransportInOutUpdate(HazyDatagramTransportInOut* self);
```
