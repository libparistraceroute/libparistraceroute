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
	$(MAKE) distdir ; \
	tar cvjf $(PACKAGE)-$(VERSION).tar.bz2 $(PACKAGE)-$(VERSION) ; \
	$(RM) -rf $(PACKAGE)-$(VERSION) ; \
	cp $(PACKAGE)-$(VERSION).tar.bz2 packages/rpm/SOURCES/ ; \
	rpmbuild -ba --clean packages/rpm/SPECS/$(PACKAGE).spec ; \
	rpmbuild -ba --clean --rmsource packages/rpm/SPECS/libparistraceroute.spec ; \
	rm -f $(PACKAGE)-$(VERSION).tar.bz2
