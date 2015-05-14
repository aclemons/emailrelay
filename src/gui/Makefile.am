#
## Copyright (C) 2001-2015 Graeme Walker <graeme_walker@users.sourceforge.net>
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
# Builds emailrelay-gui.real.
#
# The following additional targets can be used for packaging:
#
# payload
#
#	Builds the install payload tree ("payload/") by collecting together 
#	the result of running "make install" into a temporary directory. 
#   Use "make MAKE_SETUP_DEBUG=-d payload" to stop the temporary 
#   directory getting cleaned up.
#
# setup (pseudo-target) [Mac]
#
#	Assembles the install bundle ("E-MailRelay-Configure.app"), which is
#   the same as the configure bundle with the extra payload.
#
# image (pseudo-target) [Mac]
#
#	Builds the installation disk image ("E-MailRelay.dmg") from the install
#	bundle ("E-MailRelay-Install.app/") using hdiutil.
#

CONFIGURE_BUNDLE=E-MailRelay-Configure
INSTALL_BUNDLE=E-MailRelay-Install
IMAGE=E-MailRelay.dmg

if MAC
MAC_EXTRA_DIST = dir_unix.cpp boot_unix.cpp glink_unix.cpp glink_win32.cpp gregister_unix.cpp gregister_win32.cpp emailrelay-icon.icns
MAC_SOURCES = dir_mac.cpp boot_mac.cpp glink_mac.cpp gregister_mac.cpp
MAC_NOINST = $(CONFIGURE_BUNDLE).app
MAC_PROGS = emailrelay-start-gui
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

moc_gdialog.cpp: $(srcdir)/gdialog.h
	$(QT_MOC) $(srcdir)/gdialog.h -o $@

moc_gpage.cpp: $(srcdir)/gpage.h
	$(QT_MOC) $(srcdir)/gpage.h -o $@

moc_pages.cpp: $(srcdir)/pages.h
	$(QT_MOC) $(srcdir)/pages.h -o $@

CLEANFILES = $(MOC_OUTPUT)

AM_CPPFLAGS=\
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
	-DG_EXAMPLESDIR=\"$(e_examplesdir)\" \
	$(QT_CFLAGS)

sbin_PROGRAMS = emailrelay-gui.real $(MAC_PROGS)
noinst_SCRIPTS = $(MAC_NOINST)

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
	installer.cpp \
	installer.h \
	legal.cpp \
	legal.h \
	mapfile.cpp \
	mapfile.h \
	pages.cpp \
	pages.h \
	qt.h \
	serverconfiguration.cpp \
	serverconfiguration.h

emailrelay_gui_real_DEPENDENCIES = $(MOC_OBJECTS)

emailrelay_gui_real_LDADD = \
	$(MOC_OBJECTS) \
	$(top_builddir)/src/glib/libglib.a \
	$(QT_LIBS)

emailrelay_start_gui_SOURCES = guistart.c
emailrelay_start_gui_LDADD =

install-exec-hook:
	-mkdir -p "$(DESTDIR)$(sbindir)" 2>/dev/null
	@echo "#!/bin/sh" > "$(DESTDIR)$(sbindir)/emailrelay-gui"
	@echo exec $(DESTDIR)$(sbindir)/emailrelay-gui.real \"$$\@\" \"$0\" >> "$(DESTDIR)$(sbindir)/emailrelay-gui"
	@echo "dir-install"=\"$(DESTDIR)/usr\" >> "$(DESTDIR)$(sbindir)/emailrelay-gui"
	@echo "dir-spool"=\"$(DESTDIR)$(e_spooldir)\" >> "$(DESTDIR)$(sbindir)/emailrelay-gui"
	@echo "dir-config"=\"$(DESTDIR)$(e_sysconfdir)\" >> "$(DESTDIR)$(sbindir)/emailrelay-gui"
	@chmod +x "$(DESTDIR)$(sbindir)/emailrelay-gui"
	-if test -d "$(CONFIGURE_BUNDLE).app" ; then cp -R "$(CONFIGURE_BUNDLE).app" "$(DESTDIR)$(sbindir)/" ; cp "$(DESTDIR)$(sbindir)/emailrelay-gui" "$(DESTDIR)$(sbindir)/$(CONFIGURE_BUNDLE).app/Contents/MacOS/$(CONFIGURE_BUNDLE).cfg" ; fi

uninstall-hook:
	-rm -f "$(DESTDIR)$(sbindir)/emailrelay-gui"
	-rm -rf "$(DESTDIR)$(sbindir)/$(CONFIGURE_BUNDLE).app"

clean-local:
	-rm -rf "$(CONFIGURE_BUNDLE).app"
	-rm -rf "$(INSTALL_BUNDLE).app"
	-rm -f "$(IMAGE)"
	-rm -f payload

else

install-exec-hook:

uninstall-hook:

endif

install-data-local: install-e_iconDATA

uninstall-local:
	-rmdir "$(DESTDIR)$(e_icondir)" 2>/dev/null

.PHONY: setup
setup: $(INSTALL_BUNDLE).app

.PHONY: image
image: $(IMAGE)

payload: ../../bin/make-setup.sh
	-mkdir payload 2>/dev/null
	../../bin/make-setup.sh $(MAKE_SETUP_DEBUG) $@ "$(top_srcdir)/src/gui/emailrelay-icon.png"

$(CONFIGURE_BUNDLE).app: emailrelay-gui.real ../../bin/make-bundle.sh ../../bin/make-qt-enabled.sh
	../../bin/make-bundle.sh -f "$(CONFIGURE_BUNDLE)" emailrelay-gui.real "$(top_srcdir)/src/gui/emailrelay-icon.icns"
	../../bin/make-qt-enabled.sh "$(QT_MOC)" "$(CONFIGURE_BUNDLE).app/Contents/MacOS/$(CONFIGURE_BUNDLE)"

$(INSTALL_BUNDLE).app: emailrelay-gui.real payload ../../bin/make-bundle.sh ../../bin/make-qt-enabled.sh
	../../bin/make-bundle.sh -f "$(INSTALL_BUNDLE)" emailrelay-gui.real "$(top_srcdir)/src/gui/emailrelay-icon.icns"
	../../bin/make-qt-enabled.sh "$(QT_MOC)" "$(INSTALL_BUNDLE).app/Contents/MacOS/$(INSTALL_BUNDLE)"
	cp -r payload "$(INSTALL_BUNDLE).app/Contents/Resources"

$(IMAGE): $(INSTALL_BUNDLE).app
	hdiutil create -srcfolder "$(INSTALL_BUNDLE).app" "$(IMAGE)" && hdiutil internet-enable -yes "$(IMAGE)"

