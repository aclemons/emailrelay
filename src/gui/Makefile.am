#
## Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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

EXTRA_DIST=\
	Info.plist \
	emailrelay-icon.png \
	gcominit.h \
	dir_win32.cpp \
	mingw.mak \
	mac.mak \
	pack.cpp \
	run.c \
	mock

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

sbin_PROGRAMS = emailrelay-gui
noinst_SCRIPTS = mock

emailrelay_gui_SOURCES = \
	gdialog.cpp \
	gdialog.h \
	glink.h \
	glink.cpp \
	gpage.cpp \
	gpage.h \
	gunpack.h \
	gunpack.cpp \
	dir.cpp \
	dir_unix.cpp \
	dir.h \
	boot.cpp \
	boot.h \
	installer.cpp \
	installer.h \
	guimain.cpp \
	legal.cpp \
	legal.h \
	pages.cpp \
	pages.h \
	qt.h \
	unpack.c \
	unpack.h

emailrelay_gui_DEPENDENCIES = $(MOC_OBJECTS)

emailrelay_gui_LDADD = \
	$(MOC_OBJECTS) \
	$(top_builddir)/src/glib/libglib.a \
	$(QT_LIBS)


# tell the configuration gui where make install has put everything -- see Dir::read() in dir.cpp
install-exec-hook:
	echo $(DESTDIR)$(e_spooldir) > $(DESTDIR)$(sbindir)/emailrelay-gui.state
	echo $(DESTDIR)$(e_sysconfdir) >> $(DESTDIR)$(sbindir)/emailrelay-gui.state

uninstall-hook:
	-rm $(DESTDIR)$(sbindir)/emailrelay-gui.state

else

install-exec-hook:

uninstall-hook:

endif
