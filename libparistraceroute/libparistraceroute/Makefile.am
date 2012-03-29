@SET_MAKE@

AUTOMAKE_OPTIONS = foreign

###############################################################################
#
# THE LIBRARIES TO BUILD
#

# the library names to build (note we are building static libs only)
lib_LIBRARIES = libparistraceroute.a

# lib_LTLIBRARIES = libparistraceroute.la 

# where to install the headers on the system
libparistraceroute_adir = $(includedir)/paristraceroute

# the list of header files that belong to the library (to be installed later)
libparistraceroute_a_HEADERS =  \
						algorithm.h \
						algorithms/mda.h \
						algorithms/traceroute.h \
						bitfield.h \
						buffer.h \
						config.h \
						dynarray.h \
						event.h \
						field.h \
						layer.h \
						list.h \
						metafield.h \
						network.h \
						packet.h \
						paristraceroute.h \
						probe.h \
						protocol.h \
						protocol_field.h \
						pseudoheader.h \
						pt_loop.h \
						queue.h \
						sniffer.h \
						socketpool.h

# the sources to add to the library and to add to the source distribution
libparistraceroute_a_SOURCES =    \
                        $(libparistraceroute_a_HEADERS) \
						algorithm.c \
						algorithms/mda.c \
						algorithms/traceroute.c \
						bitfield.c \
						buffer.c \
						dynarray.c \
						event.c \
						field.c \
						layer.c \
						list.c \
						metafield.c \
						network.c \
						packet.c \
						paristraceroute.c \
						probe.c \
						protocol.c \
						protocols/udp.c \
						pt_loop.c \
						queue.c \
						sniffer.c \
						socketpool.c

#include_HEADERS = paristraceroute.h
#
## Sources
#libparistraceroute_la_SOURCES = paristraceroute.h algo.c algo.h \
#algo_simplehop.c field.c field.h fieldlist.c fieldlist.h instance.c instance.h \
#Makefile.am network.c network.h output.c output.h output_simple.c packet.c \
#packet.h probe.c probe.h proto.c proto.h proto_icmp.c proto_ipv4.c proto_udp.c \
#queue.c queue.h sniffer.c sniffer.h traceroute.c traceroute.h hastable.h algo_mda.c
#
#libparistraceroute_la_LDFLAGS = -version-info 0:0:0
