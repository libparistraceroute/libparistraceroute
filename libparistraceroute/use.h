#ifndef USE_H

// This header allows to enable or disable some functionnalities of libparistraceroute
// in order to get a smaller binary.

// Enable IPv4 and IPv6 support
#define USE_IPV4
#define USE_IPV6

// Enable bit-level fields and non-aligned fields management
#define USE_BITS

// Enable caches (for example, DNS caches)
#define USE_CACHE

// Enable scheduling of probes
#define USE_SCHEDULING

// Enable JSON/XML format
#define USE_FORMAT_JSON
#define USE_FORMAT_XML

#endif
