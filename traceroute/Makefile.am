@SET_MAKE@

AUTOMAKE_OPTIONS = foreign

###############################################################################
#
# THE PROGRAMS TO BUILD
#

# the program to build (the names of the final binaries)
noinst_PROGRAMS = traceroute

# list of sources for the traceroute binary
traceroute_SOURCES = \
	traceroute.c

traceroute_CFLAGS = \
	$(AM_CFLAGS) \
	-I$(srcdir)/../libparistraceroute

traceroute_LDADD = \
	../libparistraceroute/libparistraceroute-@LIBRARY_VERSION@.la

traceroute_LDFLAGS = \
	$(AM_LDFLAGS) \
	-L../libparistraceroute

