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
# Makefile.am
#
# Builds emailrelay-gui.real and a no-install "pack" utility (amoungst
# other things).
#
# The following additional targets can be used for packaging:
#
# payload
#
#	Builds the payload file ("payload") by pack-ing the all stuff that
#	needs to be installed. The emailrelay-gui.real installer looks for
#	this if it does not have its own payload packed on the end.
#
#	The stuff to be installed comes from running "make install" into
#	a temporary directory. Use "make MAKE_SETUP_DEBUG=-d payload" to
#	stop the temporary directory getting cleaned up.
#
# emailrelay-setup
#
#	Builds a self-extracting setup program ("emailrelay-setup") from
#	emailrelay-gui.real with the payload stuff packed on the end.
#
# univeral (pseudo-target) [Mac]
#
#	Overwrites makefile-built binaries with out-of-tree ones. Typically
#	used on Mac OS X after doing an xcode build of universal binaries.
#
# setup-bundle (pseudo-target) [Mac]
#
#	Assembles the install bundle ("E-MailRelay-Install.app/"). Calls
#	bin/make-bundle.sh to create the directory structure and the
#	plist file.
#
# image (pseudo-target) [Mac]
#
#	Builds the installation disk image ("E-MailRelay.dmg") from the install
#	bundle ("E-MailRelay-Install.app/") using hdiutil.
#

BUNDLE=E-MailRelay-Configure
INSTALL_BUNDLE=E-MailRelay-Install
IMAGE=E-MailRelay.dmg

if MAC
MAC_EXTRA_DIST = dir_unix.cpp boot_unix.cpp glink_unix.cpp glink_win32.cpp gregister_unix.cpp gregister_win32.cpp emailrelay-icon.icns
MAC_SOURCES = dir_mac.cpp boot_mac.cpp glink_mac.cpp gregister_mac.cpp
MAC_NOINST = $(BUNDLE).app
MAC_PROGS = emailrelay-start-gui emailrelay-unpack
MAKE_SETUP_STUB = NONE
else
MAC_EXTRA_DIST = dir_mac.cpp boot_mac.cpp glink_mac.cpp glink_win32.cpp gregister_mac.cpp gregister_win32.cpp guistart.c emailrelay-icon.icns
MAC_SOURCES = dir_unix.cpp boot_unix.cpp glink_unix.cpp gregister_unix.cpp
MAC_PROGS =
MAC_NOINST =
MAKE_SETUP_STUB = emailrelay-gui.real
endif

EXTRA_DIST=\
	boot_win32.cpp \
	dir_win32.cpp \
	gcominit.h \
	glink_win32.cpp \
	mingw.mak \
	mock \
	pack.cpp \
	README \
	emailrelay-icon.png \
	emailrelay-gui.rc \
	$(MAC_EXTRA_DIST)

e_icon_DATA=\
	emailrelay-icon.png

if GUI

# force symbol stripping on 'make install' -- see also make 'install-strip'
AM_INSTALL_PROGRAM_FLAGS=-s

MOC_OUTPUT=\
	moc_gdialog.cpp \
	moc_gpage.cpp \
	moc_pages.cpp

MOC_OBJECTS=\
	moc_gdialog.o \
	moc_gpage.o \
	moc_pages.o

moc_gdialog.cpp: $(srcdir)/gdialog.h $(mock)
	$(MOC) $(srcdir)/gdialog.h -o $@

moc_gpage.cpp: $(srcdir)/gpage.h $(mock)
	$(MOC) $(srcdir)/gpage.h -o $@

moc_pages.cpp: $(srcdir)/pages.h $(mock)
	$(MOC) $(srcdir)/pages.h -o $@

CLEANFILES = $(MOC_OUTPUT) $(mock)

INCLUDES=\
	-I$(top_srcdir)/lib/$(COMPILER_VERSION) \
	-I$(top_srcdir)/src/glib \
	-I$(top_srcdir)/src/gnet \
	-I$(top_srcdir)/src/gsmtp \
	-I$(top_srcdir)/src/gpop \
	-I$(top_srcdir)/src/main \
	-DG_SBINDIR=\"$(sbindir)\" \
	-DG_SPOOLDIR=\"$(e_spooldir)\" \
	-DG_INITDIR=\"$(e_initdir)\" \
	-DG_SYSCONFDIR=\"$(e_sysconfdir)\" \
	-DG_ICONDIR=\"$(e_icondir)\" \
	-DG_LIBEXECDIR=\"$(e_libexecdir)\" \
	-DG_MANDIR=\"$(mandir)\" \
	-DG_DOCDIR=\"$(e_docdir)\" \
	-DG_EXAMPLESDIR=\"$(e_examplesdir)\"

AM_CPPFLAGS=$(QT_CFLAGS)

sbin_PROGRAMS = emailrelay-gui.real $(MAC_PROGS)
noinst_SCRIPTS = mock $(MAC_NOINST)
noinst_PROGRAMS = pack emailrelay-unpack

emailrelay_gui_real_SOURCES = \
	$(MAC_SOURCES) \
	boot.h \
	dir.cpp \
	dir.h \
	gdialog.cpp \
	gdialog.h \
	glink.h \
	gregister.h \
	gpage.cpp \
	gpage.h \
	guimain.cpp \
	gunpack.cpp \
	gunpack.h \
	installer.cpp \
	installer.h \
	legal.cpp \
	legal.h \
	mapfile.cpp \
	mapfile.h \
	pages.cpp \
	pages.h \
	pointer.cpp \
	pointer.h \
	qt.h \
	state.cpp \
	state.h \
	unpack.c \
	unpack.h

emailrelay_gui_real_DEPENDENCIES = $(MOC_OBJECTS)

emailrelay_gui_real_LDADD = \
	$(MOC_OBJECTS) \
	$(top_builddir)/src/glib/libglib.a \
	$(QT_LIBS)

pack_SOURCES = pack.cpp
pack_LDADD = $(top_builddir)/src/glib/libglib.a $(ZLIB_LIBS)

emailrelay_unpack_SOURCES = unpackmain.c unpack.c
emailrelay_unpack_LDADD = $(ZLIB_LIBS)

emailrelay_start_gui_SOURCES = guistart.c unpack.c
emailrelay_start_gui_LDADD =

install-exec-hook:
	@echo "#!/bin/sh" > "$(DESTDIR)$(sbindir)/emailrelay-gui"
	@echo DIR_INSTALL=\"$(DESTDIR)/usr\" >> "$(DESTDIR)$(sbindir)/emailrelay-gui"
	@echo DIR_SPOOL=\"$(DESTDIR)$(e_spooldir)\" >> "$(DESTDIR)$(sbindir)/emailrelay-gui"
	@echo DIR_CONFIG=\"$(DESTDIR)$(e_sysconfdir)\" >> "$(DESTDIR)$(sbindir)/emailrelay-gui"
	@echo exec $(DESTDIR)$(sbindir)/emailrelay-gui.real \"$$\@\" >> "$(DESTDIR)$(sbindir)/emailrelay-gui"
	@chmod +x "$(DESTDIR)$(sbindir)/emailrelay-gui"
	-if test -d "$(BUNDLE).app" ; then cp -R "$(BUNDLE).app" "$(DESTDIR)$(sbindir)" ; fi

uninstall-hook:
	-rm "$(DESTDIR)$(sbindir)/emailrelay-gui"
	-rm -rf "$(DESTDIR)$(sbindir)/$(BUNDLE).app"

clean-local:
	-rm -rf "$(BUNDLE).app"
	-rm -rf "$(INSTALL_BUNDLE).app"
	-rm -f emailrelay-setup payload
	-rm -f "$(IMAGE)"
	-rm -f "emailrelay-setup.state"

else

install-exec-hook:

uninstall-hook:

endif

install-data-local: install-e_iconDATA

uninstall-local:
	-rmdir "$(DESTDIR)$(e_icondir)" 2>/dev/null

$(BUNDLE).app: emailrelay-start-gui
	../../bin/make-bundle.sh -f "$(BUNDLE)" emailrelay-start-gui "$(top_srcdir)/src/gui/emailrelay-icon.icns"

.PHONY: bundle
bundle: $(BUNDLE).app

emailrelay-setup: pack $(MAKE_SETUP_STUB) ../../bin/make-setup.sh
	../../bin/make-setup.sh $(MAKE_SETUP_DEBUG) $@ $(MAKE_SETUP_STUB) pack "$(top_srcdir)/src/gui/emailrelay-icon.png"
	@chmod +x $@

payload: pack ../../bin/make-setup.sh
	../../bin/make-setup.sh $(MAKE_SETUP_DEBUG) $@ NONE pack "$(top_srcdir)/src/gui/emailrelay-icon.png"

$(INSTALL_BUNDLE).app: emailrelay-gui.real payload ../../bin/make-bundle.sh
	../../bin/make-bundle.sh -f "$(INSTALL_BUNDLE)" emailrelay-gui.real "$(top_srcdir)/src/gui/emailrelay-icon.icns"
	mv "$(INSTALL_BUNDLE).app/Contents/MacOS/E-MailRelay-Install" "$(INSTALL_BUNDLE).app/Contents/emailrelay-gui.real"
	( cd "$(INSTALL_BUNDLE).app/Contents/MacOS" && ln -s ../emailrelay-gui.real E-MailRelay-Install )
	../../bin/make-qt-enabled.sh -f "$(INSTALL_BUNDLE).app/Contents/emailrelay-gui.real" "Frameworks"
	cp payload "$(INSTALL_BUNDLE).app/Contents/"

.PHONY: setup-bundle
setup-bundle: $(INSTALL_BUNDLE).app

$(IMAGE): $(INSTALL_BUNDLE).app emailrelay-unpack
	-@rm -f "$(INSTALL_BUNDLE).app/Contents/MacOS/$(INSTALL_BUNDLE).state"
	cp emailrelay-unpack "$(INSTALL_BUNDLE).app/Contents/"
	hdiutil create -srcfolder "$(INSTALL_BUNDLE).app" "$(IMAGE)" && hdiutil internet-enable -yes "$(IMAGE)"

.PHONY: image
image: $(IMAGE)

.PHONY: universal
universal:
	cp ../../build/Release/emailrelay ../main/emailrelay && touch ../main/emailrelay
	cp ../../build/Release/emailrelay-filter-copy ../main/emailrelay-filter-copy && touch ../main/emailrelay-filter-copy
	cp ../../build/Release/emailrelay-passwd ../main/emailrelay-passwd && touch ../main/emailrelay-passwd
	cp ../../build/Release/emailrelay-poke ../main/emailrelay-poke && touch ../main/emailrelay-poke
	cp ../../build/Release/emailrelay-start ../main/emailrelay-start && touch ../main/emailrelay-start
	cp ../../build/Release/emailrelay-submit ../main/emailrelay-submit && touch ../main/emailrelay-submit
	cp ../../build/Release/emailrelay-gui ../gui/emailrelay-gui.real && touch ../gui/emailrelay-gui.real
	cp ../../build/Release/emailrelay-start-gui ../gui/emailrelay-start-gui && touch ../gui/emailrelay-start-gui
	cp ../../build/Release/emailrelay-unpack ../gui/emailrelay-unpack && touch ../gui/emailrelay-unpack

