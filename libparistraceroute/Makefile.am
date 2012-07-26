@SET_MAKE@

AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I m4

# The subdirectories of the project to go into
SUBDIRS = libparistraceroute paris-traceroute traceroute man doc test

install-dev:
	cd libparistraceroute && $(MAKE) $(AM_MAKEFLAGS) install-dev

install-bin:
	cd paris-traceroute && $(MAKE) $(AM_MAKEFLAGS) install-bin
	cd man && $(MAKE) $(AM_MAKEFLAGS) install

install-lib:
	cd libparistraceroute && $(MAKE) $(AM_MAKEFLAGS) install-lib
