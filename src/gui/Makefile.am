#
## Copyright (C) 2001-2020 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#	Use "make MAKE_SETUP_DEBUG=-d payload" to stop the temporary
#	directory getting cleaned up.
#

MAC_SOURCES = access_mac.cpp dir_mac.cpp boot_mac.cpp glink_mac.cpp
WIN32_SOURCES = access_win32.cpp dir_win32.cpp boot_win32.cpp glink_win32.cpp emailrelay-gui.rc
UNIX_SOURCES = access_unix.cpp dir_unix.cpp boot_unix.cpp glink_unix.cpp

if GCONFIG_WINDOWS
OS_EXTRA_DIST = $(MAC_SOURCES) $(UNIX_SOURCES)
OS_SOURCES = $(WIN32_SOURCES)
OS_NOINST =
else
if GCONFIG_MAC
OS_EXTRA_DIST = $(UNIX_SOURCES) $(WIN32_SOURCES)
OS_SOURCES = $(MAC_SOURCES)
OS_NOINST =
else
OS_EXTRA_DIST = $(MAC_SOURCES) $(WIN32_SOURCES)
OS_SOURCES = $(UNIX_SOURCES)
OS_NOINST =
endif
endif

EXTRA_DIST=\
	emailrelay-gui.exe.manifest \
	gcominit.h \
	messages.mc \
	$(OS_EXTRA_DIST)

# force symbol stripping on 'make install' -- see also make 'install-strip'
AM_INSTALL_PROGRAM_FLAGS=-s

MOC_OUTPUT=\
	moc_gdialog.cpp \
	moc_gpage.cpp \
	moc_pages.cpp \
	moc_launcher.cpp

MOC_OBJECTS=\
	moc_gdialog.o \
	moc_gpage.o \
	moc_pages.o \
	moc_launcher.o

CLEANFILES = $(MOC_OUTPUT)

AM_CPPFLAGS=\
	-I$(top_srcdir)/src/glib \
	-I$(top_srcdir)/src/gnet \
	-I$(top_srcdir)/src/gsmtp \
	-I$(top_srcdir)/src/gpop \
	-I$(top_srcdir)/src/main \
	-D "G_SBINDIR=$(sbindir)" \
	-D "G_SPOOLDIR=$(e_spooldir)" \
	-D "G_INITDIR=$(e_initdir)" \
	-D "G_SYSCONFDIR=$(e_sysconfdir)" \
	-D "G_ICONDIR=$(e_icondir)" \
	-D "G_LIBEXECDIR=$(e_libexecdir)" \
	-D "G_MANDIR=$(mandir)" \
	-D "G_DOCDIR=$(e_docdir)" \
	-D "G_EXAMPLESDIR=$(e_examplesdir)" \
	$(GCONFIG_QT_CFLAGS)

if GCONFIG_GUI

moc_gdialog.cpp: $(srcdir)/gdialog.h
	$(GCONFIG_QT_MOC) $(srcdir)/gdialog.h -o $@

moc_gpage.cpp: $(srcdir)/gpage.h
	$(GCONFIG_QT_MOC) $(srcdir)/gpage.h -o $@

moc_pages.cpp: $(srcdir)/pages.h
	$(GCONFIG_QT_MOC) $(srcdir)/pages.h -o $@

moc_launcher.cpp: $(srcdir)/launcher.h
	$(GCONFIG_QT_MOC) $(srcdir)/launcher.h -o $@

sbin_PROGRAMS = emailrelay-gui.real
noinst_SCRIPTS = $(OS_NOINST)

emailrelay_gui_real_SOURCES = \
	$(OS_SOURCES) \
	access.h \
	boot.h \
	dir.cpp \
	dir.h \
	gdialog.cpp \
	gdialog.h \
	glink.h \
	gpage.cpp \
	gpage.h \
	guimain.cpp \
	installer.cpp \
	installer.h \
	launcher.cpp \
	launcher.h \
	legal.cpp \
	legal.h \
	pages.cpp \
	pages.h \
	qt.h \
	serverconfiguration.cpp \
	serverconfiguration.h \
	glibsources.cpp

emailrelay_gui_real_DEPENDENCIES = $(MOC_OBJECTS)

emailrelay_gui_real_LDADD = \
	$(MOC_OBJECTS) \
	$(GCONFIG_QT_LIBS)

install-exec-hook:
	-mkdir -p "$(DESTDIR)$(sbindir)" 2>/dev/null
	@echo "#!/bin/sh" > "$(DESTDIR)$(sbindir)/emailrelay-gui"
	@echo exec $(DESTDIR)$(sbindir)/emailrelay-gui.real \"$$\@\" \"$0\" >> "$(DESTDIR)$(sbindir)/emailrelay-gui"
	@echo "dir-install"=\"$(DESTDIR)/usr\" >> "$(DESTDIR)$(sbindir)/emailrelay-gui"
	@echo "dir-spool"=\"$(DESTDIR)$(e_spooldir)\" >> "$(DESTDIR)$(sbindir)/emailrelay-gui"
	@echo "dir-config"=\"$(DESTDIR)$(e_sysconfdir)\" >> "$(DESTDIR)$(sbindir)/emailrelay-gui"
	@chmod +x "$(DESTDIR)$(sbindir)/emailrelay-gui"

uninstall-hook:
	-rm -f "$(DESTDIR)$(sbindir)/emailrelay-gui"

clean-local:
	-rm -f payload

else

install-exec-hook:

uninstall-hook:

endif

payload: $(top_srcdir)/bin/make-setup.sh
	-mkdir payload 2>/dev/null
	-@chmod +x $(top_srcdir)/bin/make-setup.sh
	$(top_srcdir)/bin/make-setup.sh $(MAKE_SETUP_DEBUG) $@ "$(top_srcdir)/src/gui/emailrelay-icon.png"

