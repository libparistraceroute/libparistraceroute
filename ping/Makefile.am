@SET_MAKE@

AUTOMAKE_OPTIONS = foreign

###############################################################################
#
# THE PROGRAMS TO BUILD
#

# the program to build (the names of the final binaries)
bin_PROGRAMS = ping

# list of sources for the paris-traceroute binary
ping_SOURCES = \
	ping.c

ping_CFLAGS = \
	$(AM_CFLAGS) \
	-I$(srcdir)/../libparistraceroute

ping_LDADD = \
	../libparistraceroute/libparistraceroute-@LIBRARY_VERSION@.la
ping_LDFLAGS = -L../libparistraceroute
ping_DEPENDENCIES =
