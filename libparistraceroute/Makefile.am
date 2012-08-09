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

rpm:
	prepare-rpm rpm-i386 rpm-x86_64

prepare-rpm:
	if [ ! -d packages/rpm/RPMS ]; then
		mkdir -p packages/rpm/RPMS/i386
		mkdir -p packages/rpm/RPMS/x86_64
	fi
	if [ ! -d packages/rpm/SRPMS ]; then
		mkdir -p packages/rpm/SRPMS
	fi
	if [ ! -d packages/rpm/SOURCES ]; then
		mkdir -p packages/rpm/SOURCES
	fi
	if [ ! -d packages/rpm/BUILD ]; then
		mkdir -p packages/rpm/BUILD
	fi
	if [ ! -d packages/rpm/BUILDROOT ]; then
		mkdir -p packages/rpm/BUILDROOT
	fi
	$(MAKE) dist-bzip2 ; \
	mv $(PACKAGE)-$(VERSION).tar.bz2 packages/rpm/SOURCES/

rpm-x86_64:
	rpmbuild -ba --target=x86_64-gnu-linux --clean packages/rpm/SPECS/$(PACKAGE).spec ; \
	rpmbuild -ba --target=x86_64-gnu-linux --clean packages/rpm/SPECS/libparistraceroute.spec
rpm-i386:
	rpmbuild -ba --target=i386-gnu-linux --clean packages/rpm/SPECS/$(PACKAGE).spec ; \
	rpmbuild -ba --target=i386-gnu-linux --clean --rmsource packages/rpm/SPECS/libparistraceroute.spec
