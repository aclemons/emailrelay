#
## Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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
# Makefile.am
#
# Top-level makefile
#
# Additional pseudo-targets:
#	* rpm - builds an rpm package using rpmbuild
#	* deb - builds a deb package using debhelper
#	* cmake - generates cmake files under ./build/
#	* tidy - runs cmake-tidy
#	* format - runs cmake-format
#
# Eg:
#	$ dnf install g++ make automake openssl-devel pam-devel rpm-build # rpm
#	$ ./bootstrap
#	$ ./configure.sh
#	$ make
#	$ sudo make rpm
#
# or:
#	$ apt install g++ make automake libssl-dev libpam0g-dev debhelper # deb
#	$ ./bootstrap
#	$ ./configure.sh
#	$ make
#	$ sudo make deb
#
# and possibly:
#	$ make cmake ; make -C build
#	$ make tidy TIDY=clang-tidy-10
#	$ make format FORMAT=clang-format-10
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
	runperl.bat

SUBDIRS = libexec bin src etc doc debian test m4 bsd po unity

# work-round PKG_CHECK_MODULES if no pkg-config
ACLOCAL_AMFLAGS = -I m4

e_doc_DATA = COPYING AUTHORS INSTALL NEWS README ChangeLog

uninstall-local:
	-for try in 1 2 3 4 ; do echo "$(e_pamdir)/" "$(e_initdir)/" "$(e_sysconfdir)/" "$(e_examplesdir)/" "$(e_libexecdir)/" "$(e_spooldir)/" "$(e_docdir)/" "$(pkgdatadir)/" | tr ' ' '\n' | grep "/emailrelay/" | while read d ; do rmdir "$(DESTDIR)/$$d" 2>/dev/null ; done ; done

RPM_ROOT ?= ${HOME}/rpmbuild
#ARCH ?= $(shell arch)
ARCH ?= x86_64
RPM=$(RPM_ROOT)/RPMS/$(ARCH)/emailrelay-$(VERSION)-1.$(ARCH).rpm
TAR=emailrelay-$(VERSION).tar.gz
ZIP=emailrelay-$(VERSION)-src.zip
DEB=emailrelay_$(VERSION)_$(ARCH).deb

.PHONY: strip
strip:
	$(MAKE) -C src/main strip

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
	tar -xzf $(TAR)
	cd emailrelay-$(VERSION) && fakeroot debian/rules binary # or dpkg-buildpackage -b --jobs=10
	mkdir emailrelay-$(VERSION)/debian/source

.PHONY: deb-src
deb-src: dist
	-mv emailrelay-$(VERSION) emailrelay-$(VERSION).old.$$$$ || true
	tar -xzf $(TAR)
	mkdir emailrelay-$(VERSION)/debian/source
	echo 1.0 > emailrelay-$(VERSION)/debian/source/format
	dpkg-source --build emailrelay-$(VERSION)

.PHONY: tidy
tidy:
	$(MAKE) -C unity tidy

.PHONY: unity
unity:
	$(MAKE) -C unity programs

.PHONY: format
format:
	$(MAKE) -C src format

.PHONY: cmake
cmake:
	@chmod +x libexec/make2cmake || true
	libexec/make2cmake
	test -d build || mkdir build
	cd build && cmake -DCMAKE_MAKE_PROGRAM:FILEPATH=/usr/bin/make -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
	@echo now run make from the '"build"' directory

.PHONY: qmake
qmake:
	@chmod +x libexec/make2qmake || true
	libexec/make2qmake

.PHONY: tar
tar: $(TAR)

$(TAR):
	$(MAKE) dist

.PHONY: zip
zip: $(ZIP)

$(ZIP): $(TAR)
	-mkdir makezip.tmp 2>/dev/null
	-@rm -rf makezip.tmp/* 2>/dev/null
	tar -C makezip.tmp --strip-components=1 -xzf $(TAR)
	sh -c "cd makezip.tmp && zip -r ../$(ZIP) ."

.PHONY: mbedtls
mbedtls:
	test -d mbedtls || git clone https://salsa.debian.org/debian/mbedtls.git
	make -C mbedtls/library WINDOWS=0
	-@echo now run ./configure.sh

