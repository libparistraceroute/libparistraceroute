@SET_MAKE@

AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I m4

# The subdirectories of the project to go into
SUBDIRS = libparistraceroute paris-traceroute ping traceroute man doc

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

rpm-prepare:
	$(MAKE) distdir ; \
	tar cvjf $(PACKAGE)-$(VERSION).tar.bz2 $(PACKAGE)-$(VERSION) ; \
	$(RM) -rf $(PACKAGE)-$(VERSION) ; \
	cp $(PACKAGE)-$(VERSION).tar.bz2 packages/rpm/SOURCES/ ;

rpm-i386: rpm-prepare
	rpmbuild -ba --nodeps --target=i386 --clean packages/rpm/SPECS/$(PACKAGE).spec ; \
	rpmbuild -ba --nodeps --target=i386 --clean --rmsource packages/rpm/SPECS/libparistraceroute.spec ;

rpm-x86_64: rpm-prepare
	rpmbuild -ba --nodeps --target=x86_64 --clean packages/rpm/SPECS/$(PACKAGE).spec ; \
	rpmbuild -ba --nodeps --target=x86_64 --clean --rmsource packages/rpm/SPECS/libparistraceroute.spec ;

rpm-clean:
	rm -f $(PACKAGE)-$(VERSION).tar.bz2

rpm: rpm-i386 rpm-x86_64 rpm-clean
