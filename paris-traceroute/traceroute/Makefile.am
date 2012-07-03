@SET_MAKE@

AUTOMAKE_OPTIONS = foreign

###############################################################################
#
# THE PROGRAMS TO BUILD
#

# the program to build (the names of the final binaries)
bin_PROGRAMS = traceroute

# list of sources for the traceroute binary
traceroute_SOURCES =    \
                        traceroute.c

traceroute_LDADD = -lparistraceroute-1.0
traceroute_LDFLAGS = -L../libparistraceroute
traceroute_DEPENDENCIES =

install:
