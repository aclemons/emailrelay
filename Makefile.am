#
## Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#	* deb - converts the rpm to a deb package using alien.
#
# When running "make distcheck" consider doing this to avoid
# a lengthy testing stage:
#	$ make distcheck DISTCHECK_CONFIGURE_FLAGS=--disable-testing
#

EXTRA_DIST = \
	emailrelay.spec \
	ChangeLog \
	README.windows \
	README.embedded \
	README.mac \
	LICENSE \
	VERSION \
	bootstrap \
	autogen.sh \
	emailrelay-gui.xcodeproj/project.pbxproj \
	emailrelay.xcodeproj/project.pbxproj 

SUBDIRS = bin src lib etc doc debian test extra

e_doc_DATA = COPYING AUTHORS INSTALL NEWS README README.windows README.mac README.embedded ChangeLog

uninstall-local:
	-for try in 1 2 3 4 ; do echo "$(DESTDIR)$(e_pamdir)" "$(DESTDIR)$(e_initdir)" "$(DESTDIR)$(e_sysconfdir)" "$(DESTDIR)$(e_examplesdir)" "$(DESTDIR)$(e_libexecdir)" "$(DESTDIR)$(e_spooldir)" "$(DESTDIR)$(e_docdir)" "$(DESTDIR)$(pkgdatadir)" | tr ' ' '\n' | grep -i "mailrelay" | while read d ; do rmdir "$$d" 2>/dev/null ; done ; done

RPM_ROOT ?= ${HOME}/rpmbuild
#RPM_ARCH ?= $(shell arch)
RPM_ARCH ?= x86_64
RPM=$(RPM_ROOT)/RPMS/$(RPM_ARCH)/emailrelay-$(VERSION)-1.$(RPM_ARCH).rpm
DEB=emailrelay_$(VERSION)-1_$(RPM_ARCH).deb
TAR=emailrelay-$(VERSION).tar.gz

.PHONY: rpm
rpm: $(RPM)

$(RPM): dist
	-rm -rf $(RPM_ROOT)/BUILD/emailrelay-$(VERSION)
	-mkdir -p $(RPM_ROOT)/BUILD $(RPM_ROOT)/SOURCES $(RPM_ROOT)/SPECS 2>/dev/null
	cp $(TAR) $(RPM_ROOT)/SOURCES/emailrelay-$(VERSION)-src.tar.gz
	cp $(top_srcdir)/emailrelay.spec $(RPM_ROOT)/SPECS/
	chown root:root $(RPM_ROOT)/SPECS/emailrelay.spec
	rpmbuild -ba $(RPM_ROOT)/SPECS/emailrelay.spec

.PHONY: deb
deb: $(DEB)

$(DEB): $(RPM)
	-mv emailrelay-$(VERSION) emailrelay-$(VERSION).old.$$$$
	alien --to-deb --generate --single --keep-version $(RPM)
	sed -i 's/^Section:.*/Section: mail/' emailrelay-$(VERSION)/debian/control
	sed -i 's/^Maintainer:.*/Maintainer: Graeme Walker <graeme_walker@users.sourceforge.net>/' emailrelay-$(VERSION)/debian/control
	chmod +x $(top_srcdir)/debian/pre* $(top_srcdir)/debian/post*
	cp $(top_srcdir)/debian/* emailrelay-$(VERSION)/debian/
	sed -i 's:dh_installchangelogs:dh_installchangelogs ; cp etc/init.d/emailrelay debian/emailrelay.init ; dh_installinit:' emailrelay-$(VERSION)/debian/rules
	sed -i 's:dh_installdeb:dh_installdeb ; echo /etc/emailrelay.conf > debian/emailrelay/DEBIAN/conffiles:' emailrelay-$(VERSION)/debian/rules
	cd emailrelay-$(VERSION) && debian/rules binary

