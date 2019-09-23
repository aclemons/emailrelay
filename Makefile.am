#
## Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
##
## This program is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
#
# Top-level makefile
#
# Additional pseudo-targets for packaging:
#	* rpm - builds an rpm package using rpmbuild
#	* deb - builds a deb package using debhelper
#
# When running "make distcheck" consider doing this to avoid
# a lengthy testing stage:
#	$ make distcheck DISTCHECK_CONFIGURE_FLAGS=--disable-testing
#

EXTRA_DIST = \
	emailrelay.spec \
	configure.sh \
	ChangeLog \
	LICENSE \
	VERSION \
	README.md \
	README.rst \
	bootstrap \
	autogen.sh \
	winbuild.bat \
	winbuild.pl \
	winbuild.pm \
	winbuild-parser.pm \
	runperl.bat

SUBDIRS = bin src etc doc debian test m4 bsd

# work-round PKG_CHECK_MODULES if no pkg-config
ACLOCAL_AMFLAGS = -I m4

e_doc_DATA = COPYING AUTHORS INSTALL NEWS README ChangeLog

uninstall-local:
	-for try in 1 2 3 4 ; do echo "$(e_pamdir)" "$(e_initdir)" "$(e_sysconfdir)" "$(e_examplesdir)" "$(e_libexecdir)" "$(e_spooldir)" "$(e_docdir)" "$(pkgdatadir)" | tr ' ' '\n' | grep "emailrelay$" | while read d ; do rmdir "$(DESTDIR)/$$d" 2>/dev/null ; done ; done

RPM_ROOT ?= ${HOME}/rpmbuild
#ARCH ?= $(shell arch)
ARCH ?= x86_64
RPM=$(RPM_ROOT)/RPMS/$(ARCH)/emailrelay-$(VERSION)-1.$(ARCH).rpm
TAR=emailrelay-$(VERSION).tar.gz
DEB=emailrelay_$(VERSION)_$(ARCH).deb

.PHONY: rpm
rpm: $(RPM)

$(RPM): dist
	-rm -rf $(RPM_ROOT)/BUILD/emailrelay-$(VERSION)-1.$(ARCH)
	test ! -d $(RPM_ROOT)/BUILD/emailrelay-$(VERSION)-1.$(ARCH)
	-mkdir -p $(RPM_ROOT)/BUILD $(RPM_ROOT)/SOURCES $(RPM_ROOT)/SPECS 2>/dev/null
	cp $(TAR) $(RPM_ROOT)/SOURCES/emailrelay-$(VERSION)-src.tar.gz
	cp $(top_srcdir)/emailrelay.spec $(RPM_ROOT)/SPECS/
	-chown root:root $(RPM_ROOT)/SPECS/emailrelay.spec
	-rm -f $(RPM)
	test ! -f $(RPM)
	rpmbuild -ba $(RPM_ROOT)/SPECS/emailrelay.spec
	test -f $(RPM)

.PHONY: deb
deb: $(DEB)

$(DEB): dist
	-mv emailrelay-$(VERSION) emailrelay-$(VERSION).old.$$$$ || true
	tar xzf $(TAR)
	cd emailrelay-$(VERSION) && fakeroot debian/rules binary
	mkdir emailrelay-$(VERSION)/debian/source

.PHONY: deb-src
deb-src: dist
	-mv emailrelay-$(VERSION) emailrelay-$(VERSION).old.$$$$ || true
	tar xzf $(TAR)
	mkdir emailrelay-$(VERSION)/debian/source
	echo 1.0 > emailrelay-$(VERSION)/debian/source/format
	dpkg-source --build emailrelay-$(VERSION)

