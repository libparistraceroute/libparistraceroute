@SET_MAKE@

AUTOMAKE_OPTIONS = foreign

###############################################################################
#
# THE PROGRAMS TO BUILD
#

# the program to build (the names of the final binaries)
bin_PROGRAMS = paris-traceroute

# list of sources for the paris-traceroute binary
paris_traceroute_SOURCES = \
	paris-traceroute.c

paris_traceroute_CFLAGS = \
	$(AM_CFLAGS) \
	-I$(srcdir)/../libparistraceroute

paris_traceroute_LDADD = \
	../libparistraceroute/libparistraceroute-@LIBRARY_VERSION@.la
