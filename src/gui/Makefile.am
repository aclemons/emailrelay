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
# NOT FINISHED
#

# force symbol stripping on 'make install' -- see also make 'install-strip'
AM_INSTALL_PROGRAM_FLAGS=-s

REAL_SOURCES=\
	gdialog.cpp \
	gdialog.h \
	gpage.cpp \
	gpage.h \
	dir.cpp \
	dir.h \
	main.cpp \
	legal.cpp \
	legal.h \
	pages.cpp \
	pages.h \
	qt.h \
	thread.cpp \
	thread.h

EXTRA_DIST=\
	$(REAL_SOURCES) \
	mingw.mak \
	mac.mak \
	mock_ \
	readme.txt \
	Info.plist

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

mock=mock
mock_in=$(srcdir)/mock_

moc_gdialog.cpp: $(srcdir)/gdialog.h $(mock)
	$(MOC) $< -o $@

moc_gpage.cpp: $(srcdir)/gpage.h $(mock)
	$(MOC) $< -o $@

moc_pages.cpp: $(srcdir)/pages.h $(mock)
	$(MOC) $< -o $@

moc_thread.cpp: $(srcdir)/thread.h $(mock)
	$(MOC) $< -o $@

$(mock): $(mock_in)
	cat $< > $*.tmp && mv $*.tmp $@
	chmod ugo+x $@

CLEANFILES = $(MOC_OUTPUT) $(mock)

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

noinst_SCRIPTS=mock

bin_PROGRAMS = emailrelay-gui emailrelay-install-tool

emailrelay_install_tool_SOURCES = tool.cpp
emailrelay_install_tool_LDADD = \
	$(top_builddir)/src/glib/libglib.a

emailrelay_gui_SOURCES = all.cpp

emailrelay_gui_DEPENDENCIES = all.cpp $(MOC_OBJECTS)
EXTRA_emailrelay_gui_SOURCES =

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

