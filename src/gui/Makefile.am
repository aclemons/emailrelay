#
## Copyright (C) 2001-2006 Graeme Walker <graeme_walker@users.sourceforge.net>
## 
## This program is free software; you can redistribute it and/or
## modify it under the terms of the GNU General Public License
## as published by the Free Software Foundation; either
## version 2 of the License, or (at your option) any later
## version.
## 
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
## 
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
## 
#
#
# Makefile.am
#

if GUI

# force symbol stripping on 'make install' -- see also make 'install-strip'
AM_INSTALL_PROGRAM_FLAGS=-s

GUI_SOURCES=\
	gdialog.cpp \
	gdialog.h \
	gpage.cpp \
	gpage.h \
	dir.cpp \
	dir_unix.cpp \
	dir.h \
	main.cpp \
	legal.cpp \
	legal.h \
	package.h \
	pages.cpp \
	pages.h \
	qt.h \
	thread.cpp \
	thread.h \
	unpack.c \
	unpack.h

EXTRA_DIST=\
	emailrelay-icon.png \
	gcominit.h \
	glink.h \
	glink.cpp \
	dir_win32.cpp \
	dir_winxp.cpp \
	dir_winnt.cpp \
	mingw.mak \
	mac.mak \
	readme.txt \
	Info.plist

PACKED_OUTPUT_FILES=\
	emailrelay-install-tool.tmp \
	emailrelay-gui.tmp

MOC_OUTPUT=\
	moc_gdialog.cpp \
	moc_gpage.cpp \
	moc_pages.cpp \
	moc_thread.cpp

MOC_OBJECTS=\
	moc_gdialog.o \
	moc_gpage.o \
	moc_pages.o \
	moc_thread.o

moc_gdialog.cpp: $(srcdir)/gdialog.h $(mock)
	$(MOC) $< -o $@

moc_gpage.cpp: $(srcdir)/gpage.h $(mock)
	$(MOC) $< -o $@

moc_pages.cpp: $(srcdir)/pages.h $(mock)
	$(MOC) $< -o $@

moc_thread.cpp: $(srcdir)/thread.h $(mock)
	$(MOC) $< -o $@

CLEANFILES = $(PACKED_OUTPUT_FILES) $(MOC_OUTPUT) $(mock)

INCLUDES=\
	-I$(top_srcdir)/lib/$(COMPILER_VERSION) \
	-I$(top_srcdir)/src/glib \
	-I$(top_srcdir)/src/gnet \
	-I$(top_srcdir)/src/gsmtp \
	-I$(top_srcdir)/src/gpop \
	-DG_SPOOLDIR=\"$(e_spooldir)\" \
	-DG_SYSCONFDIR=\"$(sysconfdir)\" \
	-DG_LIBEXECDIR=\"$(libexecdir)\" \
	-DG_DESTDIR=\"$(DESTDIR)\"

AM_CPPFLAGS=\
	-I$(e_qtdir)/include \
	-I$(e_qtdir)/include/Qt \
	-I$(e_qtdir)/include/QtCore \
	-I$(e_qtdir)/include/QtGui

bin_PROGRAMS = emailrelay-gui emailrelay-install-tool
noinst_SCRIPTS = mock emailrelay-setup
noinst_PROGRAMS = run pack

pack_SOURCES = pack.cpp
pack_LDADD = $(top_builddir)/src/glib/libglib.a
pack_LDFLAGS = -lz

run_SOURCES = run.c unpack.c
run_LDADD =
run_LDFLAGS = -lz

emailrelay_install_tool_SOURCES = tool.cpp glink.cpp unpack.c
emailrelay_install_tool_LDADD = \
	$(top_builddir)/src/glib/libglib.a

emailrelay_gui_SOURCES = $(GUI_SOURCES)

emailrelay_gui_DEPENDENCIES = $(MOC_OBJECTS)

emailrelay_gui_LDADD = \
	$(MOC_OBJECTS) \
	$(top_builddir)/src/glib/libglib.a \
	-lQtGui -lQtCore \
	-lpng \
	-lX11 -lXrender -lXft -lSM -lXi \
	-lXrandr -lXinerama -lXcursor \
	-lpthread

emailrelay_gui_LDFLAGS = \
	-L$(e_qtdir)/lib \
	-L$(e_x11dir)/lib

# (for *nix we assume 'make install' does the install, so this is a short list...)
PACKED_INPUT = \
	emailrelay-icon.png

PACKED_INPUT_LIST = \
	emailrelay-icon.png emailrelay-icon.png

# emailrelay-install-tool.tmp := emailrelay-install-tool, emailrelay, etc. etc.
#
emailrelay-install-tool.tmp: pack emailrelay-install-tool $(PACKED_INPUT)
	./pack $@ emailrelay-install-tool $(PACKED_INPUT_LIST)

# emailrelay-gui.tmp := emailrelay-gui + emailrelay-install-tool.tmp
#
emailrelay-gui.tmp: pack emailrelay-gui emailrelay-install-tool.tmp
	./pack $@ emailrelay-gui emailrelay-install-tool.tmp emailrelay-install-tool

# emailrelay-setup := run + emailrelay-gui.tmp
#
emailrelay-setup: pack run emailrelay-gui.tmp
	./pack $@ run emailrelay-gui.tmp emailrelay-gui
	chmod +x $@

else

EXTRA_DIST=\
	dir.cpp \
	dir.h \
	dir_unix.cpp \
	dir_win32.cpp \
	dir_winnt.cpp \
	dir_winxp.cpp \
	emailrelay-icon.png \
	gcominit.h \
	gdialog.cpp \
	gdialog.h \
	glink.cpp \
	glink.h \
	gpage.cpp \
	gpage.h \
	Info.plist \
	legal.cpp \
	legal.h \
	mac.mak \
	main.cpp \
	mingw.mak \
	pack.cpp \
	pages.cpp \
	pages.h \
	qt.h \
	readme.txt \
	thread.cpp \
	thread.h

endif
