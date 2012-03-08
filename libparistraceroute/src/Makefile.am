@SET_MAKE@

AUTOMAKE_OPTIONS = foreign

## libtraceroute
lib_LTLIBRARIES = libparistraceroute.la
include_HEADERS = paristraceroute.h

# Sources
libparistraceroute_la_SOURCES = paristraceroute.h algo.c algo.h \
algo_simplehop.c field.c field.h fieldlist.c fieldlist.h instance.c instance.h \
Makefile.am network.c network.h output.c output.h output_simple.c packet.c \
packet.h probe.c probe.h proto.c proto.h proto_icmp.c proto_ipv4.c proto_udp.c \
queue.c queue.h sniffer.c sniffer.h traceroute.c traceroute.h hastable.h algo_mda.c

libparistraceroute_la_LDFLAGS = -version-info 0:0:0
