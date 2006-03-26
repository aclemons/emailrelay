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
# mingw.mak
#
# See ../mingw-common.mak for help.
#

.PHONY: all

mk_sources=\
	gdialog.cpp \
	gpage.cpp \
	dir.cpp \
	main.cpp \
	legal.cpp \
	pages.cpp \
	moc_gdialog.cpp \
	moc_gpage.cpp \
	moc_pages.cpp

qtlib=$(mk_qt)/lib/libQt
qt_libs_debug= \
	$(qtlib)Guid.a \
	$(qtlib)Cored.a \
	$(qtlib)maind.a
qt_libs_release= \
	$(qtlib)Gui4.a \
	$(qtlib)Core4.a \
	$(qtlib)main.a

libs=../glib/glib.a $(qt_libs_release)

syslibs=-lgdi32 -lwsock32 -lole32 -loleaut32 -lwinspool -lcomdlg32 -limm32 -lwinmm -luuid
mk_target=emailrelay-gui.exe
mk_includes_extra=-I$(mk_qt)/include -I$(mk_qt)/include/QtCore -I$(mk_qt)/include/QtGui

all: $(mk_target)

include ../mingw-common.mak

$(mk_target): $(mk_objects) $(libs)
	$(mk_link) $(mk_link_flags) -o $(mk_target) $(mk_objects) $(libs) $(syslibs)

moc_gdialog.cpp: gdialog.h
	$(mk_qt)/bin/moc $<  -o $@

moc_gpage.cpp: gpage.h
	$(mk_qt)/bin/moc $< -o $@

moc_pages.cpp: pages.h
	$(mk_qt)/bin/moc $< -o $@

