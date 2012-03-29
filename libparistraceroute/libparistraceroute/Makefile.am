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
								paristraceroute.h

# the sources to add to the library and to add to the source distribution
libparistraceroute_a_SOURCES =    \
                        $(libparistraceroute_a_HEADERS) \
						algorithm.c \
						algorithm.h \
						buffer.c \
						buffer.h \
						config.h \
						dynarray.c \
						dynarray.h \
						event.c \
						event.h \
						field.c \
						field.h \
						layer.c \
						layer.h \
						list.c \
						list.h \
						network.c \
						network.h \
						packet.c \
						packet.h \
						paristraceroute.c \
						paristraceroute.h \
						probe.c \
						probe.h \
						protocol.c \
						protocol.h \
						protocols/udp.c
						protocol_field.h \
						pseudoheader.h \
						pt_loop.c \
						pt_loop.h \
						queue.c \
						queue.h \
						sniffer.c \
						sniffer.h \
						socketpool.c \
						socketpool.h

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
