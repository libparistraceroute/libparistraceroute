@SET_MAKE@

AUTOMAKE_OPTIONS = foreign
## Place generated object files (.o) into the same directory as their source
## files, in order to avoid collisions when non-recursive make is used.
# AUTOMAKE_OPTIONS = subdir-objects

## Additional flags to pass to aclocal when it is invoked automatically at
## make time. The ${ACLOCAL_FLAGS} variable is picked up from the environment
## to provide a way for the user to supply additional arguments.
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}

###############################################################################
#
# THE LIBRARIES TO BUILD
#

## Define a libtool archive target "libexample-@API_VERSION@.la", with
## @API_VERSION@ substituted into the generated Makefile at configure
## time.
## The libtool archive file (.la) will be installed into the directory named
## by the predefined variable $(bindir), along with the actual shared library
## file (.so).
lib_LTLIBRARIES = libparistraceroute-@LIBRARY_VERSION@.la

nobase_libparistraceroute_@LIBRARY_VERSION@_la_HEADERS =  \
						address.h \
                        algorithm.h \
						algorithms/mda/bound.h \
						algorithms/mda/data.h \
						algorithms/mda/flow.h \
						algorithms/mda/interface.h \
						algorithms/mda.h \
						algorithms/traceroute.h \
						bitfield.h \
						bits.h \
						buffer.h \
						common.h \
						containers/object.h \
						containers/map.h \
						containers/pair.h \
						containers/set.h \
						dynarray.h \
						event.h \
						field.h \
                        group.h \
                        generator.h \
						layer.h \
						lattice.h \
						list.h \
						metafield.h \
						network.h \
						optparse.h \
                        options.h \
						packet.h \
						probe.h \
                        probe_group.h \
						protocol.h \
						protocol_field.h \
						protocols/ipv4_pseudo_header.h \
						protocols/ipv6_pseudo_header.h \
						pt_loop.h \
						queue.h \
						sniffer.h \
						socketpool.h \
                        tree.h \
                        use.h \
                        vector.h

nodist_nobase_libparistraceroute_@LIBRARY_VERSION@_la_HEADERS = \
	config.h

# where to install the headers on the system
libparistraceroute_@LIBRARY_VERSION@_ladir = $(includedir)/paristraceroute

## Define the source file list for the "libexample-@API_VERSION@.la"
## target.  Note that @API_VERSION@ is not interpreted by Automake and
## will therefore be treated as if it were literally part of the target name,
## and the variable name derived from that.
## The file extension .cc is recognized by Automake, and makes it produce
## rules which invoke the C++ compiler to produce a libtool object file (.lo)
## from each source file.  Note that it is not necessary to list header files
## which are already listed elsewhere in a _HEADERS variable assignment.
# the list of header files that belong to the library (to be installed later)
libparistraceroute_@LIBRARY_VERSION@_la_SOURCES =    \
                        $(libparistraceroute_la_HEADERS) \
					    address.c \
                        algorithm.c \
						algorithms/mda.c \
						algorithms/mda/bound.c \
						algorithms/mda/data.c \
						algorithms/mda/flow.c \
						algorithms/mda/interface.c \
						algorithms/traceroute.c \
						bitfield.c \
						bits.c \
						buffer.c \
						common.c \
						containers/object.c \
						containers/map.c \
						containers/pair.c \
						containers/set.c \
						dynarray.c \
						event.c \
						field.c \
                        group.c \
                        generator.c \
                        generators/uniform.c \
						lattice.c \
						layer.c \
						list.c \
						metafield.c \
						network.c \
						optparse.c \
                        options.c \
						packet.c \
						probe.c \
                        probe_group.c \
						protocol.c \
						protocols/icmpv4.c \
						protocols/icmpv6.c \
						protocols/ipv4.c \
						protocols/ipv4_pseudo_header.c \
						protocols/ipv6.c \
						protocols/ipv6_pseudo_header.c \
						protocols/tcp.c \
						protocols/udp.c \
						protocol_field.c \
						pt_loop.c \
						queue.c \
						sniffer.c \
						socketpool.c \
                        tree.c \
                        vector.c

## Instruct libtool to include ABI version information in the generated shared
## library file (.so).  The library ABI version is defined in configure.ac, so
## that all version information is kept in one place.
libparistraceroute_@LIBRARY_VERSION@_la_LDFLAGS = -version-info @API_VERSION@

## Define the list of public header files and their install location.  The
## nobase_ prefix instructs Automake to not strip the directory part from each
## filename, in order to avoid the need to define separate file lists for each
## installation directory.  This only works if the directory hierarchy in the
## source tree matches the hierarchy at the install location, however.
#example_includedir = $(includedir)/example-$(API_VERSION)
#nobase_example_include_HEADERS = example.h \
#                                 example/something.h \
#                                 example/sub/sub.h

## The generated configuration header is installed in its own subdirectory of
## $(libdir).  The reason for this is that the configuration information put
## into this header file describes the target platform the installed library
## has been built for.  Thus the file must not be installed into a location
## intended for architecture-independent files, as defined by the Filesystem
## Hierarchy Standard (FHS).
## The nodist_ prefix instructs Automake to not generate rules for including
## the listed files in the distribution on 'make dist'.  Files that are listed
## in _HEADERS variables are normally included in the distribution, but the
## configuration header file is generated at configure time and should not be
## shipped with the source tarball.
#example_libincludedir = $(libdir)/example-$(API_VERSION)/include
#nodist_example_libinclude_HEADERS = exampleconfig.h

## Install the generated pkg-config file (.pc) into the expected location for
## architecture-dependent package configuration information.  Occasionally,
## pkg-config files are also used for architecture-independent data packages,
## in which case the correct install location would be $(datadir)/pkgconfig.
pkgconfigdir = $(libdir)/pkgconfig
nodist_pkgconfig_DATA = libparistraceroute-@LIBRARY_VERSION@.pc

# Dead code.
EXTRA_DIST = \
	algorithms/node_query.c \
	metafields/flow_id.c
