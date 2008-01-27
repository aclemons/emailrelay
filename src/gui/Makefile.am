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
#
# Makefile.am
#

if MAC
MAC_EXTRA_DIST=dir_unix.cpp boot_unix.cpp glink_unix.cpp
MAC_SOURCES=dir_mac.cpp boot_mac.cpp glink_mac.cpp
else
MAC_EXTRA_DIST=dir_mac.cpp boot_mac.cpp glink_mac.cpp
MAC_SOURCES=dir_unix.cpp boot_unix.cpp glink_unix.cpp
endif

EXTRA_DIST=\
	emailrelay-icon.png \
	gcominit.h \
	dir_win32.cpp \
	mingw.mak \
	pack.cpp \
	run.c \
	mock \
	$(MAC_EXTRA_DIST)

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
	$(MOC) $< -o $@

moc_gpage.cpp: $(srcdir)/gpage.h $(mock)
	$(MOC) $< -o $@

moc_pages.cpp: $(srcdir)/pages.h $(mock)
	$(MOC) $< -o $@

CLEANFILES = $(MOC_OUTPUT) $(mock)

INCLUDES=\
	-I$(top_srcdir)/lib/$(COMPILER_VERSION) \
	-I$(top_srcdir)/src/glib \
	-I$(top_srcdir)/src/gnet \
	-I$(top_srcdir)/src/gsmtp \
	-I$(top_srcdir)/src/gpop \
	-I$(top_srcdir)/src/main \
	-DG_SPOOLDIR=\"$(e_spooldir)\" \
	-DG_SYSCONFDIR=\"$(e_sysconfdir)\" \
	-DG_LIBEXECDIR=\"$(e_libexecdir)\" \
	-DG_DESTDIR=\"$(DESTDIR)\"

AM_CPPFLAGS=$(QT_CFLAGS)

sbin_PROGRAMS = emailrelay-gui.real
noinst_SCRIPTS = mock
noinst_PROGRAMS = pack

emailrelay_gui_real_SOURCES = \
	$(MAC_SOURCES) \
	boot.h \
	dir.cpp \
	dir.h \
	gdialog.cpp \
	gdialog.h \
	glink.h \
	gpage.cpp \
	gpage.h \
	guimain.cpp \
	gunpack.cpp \
	gunpack.h \
	installer.cpp \
	installer.h \
	legal.cpp \
	legal.h \
	pages.cpp \
	pages.h \
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
pack_LDADD = $(top_builddir)/src/glib/libglib.a

install-exec-hook:
	@echo "#!/bin/sh" > "$(DESTDIR)$(sbindir)/emailrelay-gui"
	@echo DIR_SPOOL=\"$(DESTDIR)$(e_spooldir)\" >> "$(DESTDIR)$(sbindir)/emailrelay-gui"
	@echo DIR_CONFIG=\"$(DESTDIR)$(e_sysconfdir)\" >> "$(DESTDIR)$(sbindir)/emailrelay-gui"
	@echo exec $(DESTDIR)$(sbindir)/emailrelay-gui.real \"$$\@\" >> "$(DESTDIR)$(sbindir)/emailrelay-gui"
	@chmod +x "$(DESTDIR)$(sbindir)/emailrelay-gui"

uninstall-hook:
	-rm "$(DESTDIR)$(sbindir)/emailrelay-gui"

else

install-exec-hook:

uninstall-hook:

endif

.PHONY: setup
setup: emailrelay-setup
emailrelay-setup: pack emailrelay-gui.real
	../../bin/make-setup.sh $(MAKE_SETUP_DEBUG) $@ emailrelay-gui.real pack
	@chmod +x $@

.PHONY: bundle
bundle:
	../../bin/make-bundle.sh "E-MailRelay Setup" emailrelay-gui.real "$(top_srcdir)/src/gui/emailrelay-icon.icns"

.PHONY: image
image: bundle
	hdiutil create -srcfolder "E-MailRelay Setup.app" "E-MailRelay.dmg" && hdiutil internet-enable -yes "E-MailRelay Setup.app"

