@SET_MAKE@

AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I m4

# The subdirectories of the project to go into
SUBDIRS = libparistraceroute paris-traceroute ping traceroute man doc

dist_noinst_SCRIPTS = \
	autogen.sh \
	clean.sh

EXTRA_DIST = \
	compil_lines.txt

# Distribute Debian package files.
EXTRA_DIST += \
	debian/changelog \
	debian/compat \
	debian/control \
	debian/copyright \
	debian/libparistraceroute-dev.dirs \
	debian/libparistraceroute-dev.install \
	debian/libparistraceroute1.dirs \
	debian/libparistraceroute1.install \
	debian/paris-traceroute.dirs \
	debian/paris-traceroute.install \
	debian/source \
	debian/source/format

dist_noinst_SCRIPTS += \
	debian/rules
