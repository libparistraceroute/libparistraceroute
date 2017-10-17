@SET_MAKE@

AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I m4

# The subdirectories of the project to go into
SUBDIRS = libparistraceroute paris-ping traceroute man doc

dist_noinst_SCRIPTS = \
	autogen.sh \
	clean.sh

# Distribute Debian package files.
EXTRA_DIST = \
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

# From jooles' branch
# - dummy.rpmmacros
install-dev:
	cd libparistraceroute && $(MAKE) $(AM_MAKEFLAGS) install-dev

install-bin:
	cd paris-traceroute && $(MAKE) $(AM_MAKEFLAGS) install-bin
	cd man && $(MAKE) $(AM_MAKEFLAGS) install

install-lib:
	cd libparistraceroute && $(MAKE) $(AM_MAKEFLAGS) install-lib

rpm:    rpm-prepare rpm-i386 rpm-x86_64 rpm-clean

rpm-prepare:
	if [ ! -d packages/rpm/RPMS ]; then \
	    mkdir -p packages/rpm/RPMS/i386 ; \
	    mkdir -p packages/rpm/RPMS/x86_64 ; \
	fi
	if [ ! -d packages/rpm/SRPMS ]; then \
	    mkdir -p packages/rpm/SRPMS ; \
	fi
	if [ ! -d packages/rpm/SOURCES ]; then \
	    mkdir -p packages/rpm/SOURCES ; \
	fi
	if [ ! -d packages/rpm/BUILD ]; then \
	    mkdir -p packages/rpm/BUILD ; \
	fi
	if [ ! -d packages/rpm/BUILDROOT ]; then \
	    mkdir -p packages/rpm/BUILDROOT ; \
	fi
	$(MAKE) dist-bzip2 ; \
	mv $(PACKAGE)-$(VERSION).tar.bz2 packages/rpm/SOURCES/ ;

rpm-host:
	rpmbuild --define '_topdir $(PWD)/packages/rpm' -ba --clean packages/rpm/SPECS/$(PACKAGE).spec ; \
	rpmbuild --define '_topdir $(PWD)/packages/rpm' -ba --clean packages/rpm/SPECS/libparistraceroute.spec ;
rpm-x86_64:
	rpmbuild --define '_topdir $(PWD)/packages/rpm' -ba --target=x86_64-gnu-linux --clean packages/rpm/SPECS/$(PACKAGE).spec ; \
	rpmbuild --define '_topdir $(PWD)/packages/rpm' -ba --target=x86_64-gnu-linux --clean packages/rpm/SPECS/libparistraceroute.spec ;
rpm-i386:
	setarch i386 rpmbuild --define '_topdir $(PWD)/packages/rpm' -ba --target=i386-gnu-linux --clean packages/rpm/SPECS/$(PACKAGE).spec ; \
	setarch i386 rpmbuild --define '_topdir $(PWD)/packages/rpm' -ba --target=i386-gnu-linux --clean packages/rpm/SPECS/libparistraceroute.spec ;
rpm-sparc:
	rpmbuild --define '_topdir $(PWD)/packages/rpm' -ba --target=sparc-gnu-linux --clean packages/rpm/SPECS/$(PACKAGE).spec ; \
	rpmbuild --define '_topdir $(PWD)/packages/rpm' -ba --target=sparc-gnu-linux --clean packages/rpm/SPECS/libparistraceroute.spec ;

rpm-clean:
	rm -rf packages/rpm/SOURCES/*

