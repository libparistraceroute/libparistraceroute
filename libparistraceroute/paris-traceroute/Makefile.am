@SET_MAKE@

AUTOMAKE_OPTIONS = foreign

###############################################################################
#
# THE PROGRAMS TO BUILD
#

# the program to build (the names of the final binaries)
bin_PROGRAMS = paris-traceroute

# list of sources for the paris-traceroute binary
paris_traceroute_SOURCES =    \
			paris-traceroute.h \
			paris-traceroute.c

#paris_traceroute_HEADERS =	\
#			paris-traceroute.h

paris_traceroute_CFLAGS = -D_GNU_SOURCE
paris_traceroute_LDADD = -lparistraceroute-1.0
paris_traceroute_LDFLAGS = -L../libparistraceroute
paris_traceroute_DEPENDENCIES =

install-bin: install
