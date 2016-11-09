@SET_MAKE@

AUTOMAKE_OPTIONS = foreign

###############################################################################
#
# THE PROGRAMS TO BUILD
#

# the program to build (the names of the final binaries)
bin_PROGRAMS = paris-ping

# list of sources for the paris-traceroute binary
paris_ping_SOURCES = \
	paris-ping.c

paris_ping_CFLAGS = \
	$(AM_CFLAGS) \
	-I$(srcdir)/../libparistraceroute

paris_ping_LDADD = \
	../libparistraceroute/libparistraceroute-@LIBRARY_VERSION@.la

install-bin: install


