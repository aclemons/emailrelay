#
## Copyright (C) 2001-2008 Graeme Walker <graeme_walker@users.sourceforge.net>
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

EXTRA_DIST = \
	emailrelay.spec \
	ChangeLog \
	README.windows \
	README.mips \
	README.mac \
	LICENSE \
	bootstrap \
	autogen.sh \
	emailrelay-gui.xcodeproj/project.pbxproj \
	emailrelay.xcodeproj/project.pbxproj 

SUBDIRS = bin src lib etc doc test extra

e_doc_DATA = NEWS README ChangeLog

uninstall-local:
	-rmdir $(DESTDIR)$(e_docdir) 2>/dev/null
	-for d in "$(DESTDIR)$(e_sysconfdir)" "$(DESTDIR)$(e_libexecdir)" "$(DESTDIR)$(e_examplesdir)" "$(DESTDIR)$(e_spooldir)" "$(DESTDIR)$(e_initdir)" "$(DESTDIR)$(e_docdir)/man/man1" "$(DESTDIR)$(e_docdir)/man" "$(DESTDIR)$(e_docdir)" ; do rmdir "$$d" 2>/dev/null ; done

RPM_ROOT=/usr/src/rpm
RPM=$(RPM_ROOT)/RPMS/i386/emailrelay-$(VERSION)-1.i386.rpm
DEB=emailrelay_$(VERSION)-1_i386.deb
TAR=emailrelay-$(VERSION).tar.gz
.PHONY: rpm
rpm: $(RPM)
$(RPM): dist
	-rm -rf $(RPM_ROOT)/BUILD/emailrelay-$(VERSION)
	cp $(TAR) $(RPM_ROOT)/SOURCES/emailrelay-$(VERSION)-src.tar.gz
	cp emailrelay.spec $(RPM_ROOT)/SPECS
	chown root:root $(RPM_ROOT)/SPECS/emailrelay.spec
	rpmbuild -ba $(RPM_ROOT)/SPECS/emailrelay.spec
.PHONY: deb
deb: $(DEB)
$(DEB): $(RPM)
	-mv emailrelay-$(VERSION) emailrelay-$(VERSION).old.$$
	alien --to-deb --generate --single --keep-version $(RPM)
	sed -i 's/^Section:.*/Section: mail/' emailrelay-$(VERSION)/debian/control
	sed -i 's/^Maintainer:.*/Maintainer: Graeme Walker <graeme_walker@users.sourceforge.net>/' emailrelay-$(VERSION)/debian/control
	cd emailrelay-$(VERSION) && debian/rules binary

