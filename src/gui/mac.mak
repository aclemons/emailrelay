#
## Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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
# mac.mak
#

.PHONY: all clean bundle install

exe=emailrelay-gui

#e_qtdir=/usr/local/Trolltech/Qt-4.1.0
qt_lib=$(e_qtdir)/lib
qt_moc=$(e_qtdir)/bin/moc

srcdir=.
top_builddir=../..

moc_objects=moc_pages.o moc_gdialog.o moc_gpage.o moc_thread.o
objects=all.o $(moc_objects)

syslibs=\
	-framework QtGui \
	-framework Carbon \
	-framework QuickTime \
	-framework QtCore \
	-lz -lm \
	-framework ApplicationServices

all: $(exe) install-tool bundle

install-tool: install-tool.cpp
	c++ -o $@ install-tool.cpp -DG_UNIX=1 -I../glib -L../glib -lglib

$(exe): $(objects)
	c++ -o $@ -F$(qt_lib) -L$(qt_lib) $(objects) -L../glib -lglib $(syslibs)

moc_%.cpp: %.h
	$(qt_moc) $< -o $@

.cpp.o:
	c++ -c $*.cpp -I../glib -DG_UNIX=1 -F$(qt_lib) -L$(qt_lib) -I../..

clean::
	-@rm -f moc_*.cpp $(objects) $(exe) install-tool .bundle 2>/dev/null

bundle: $(exe) install-tool $(srcdir)/Info.plist
	-@rm -f .bundle
	-@mkdir -p $(top_builddir)/emailrelay.app/Contents/MacOS
	cp $(srcdir)/emailrelay-gui $(top_builddir)/emailrelay.app/Contents/MacOS/
	cp $(srcdir)/install-tool $(top_builddir)/emailrelay.app/Contents/MacOS/
	-@mkdir -p $(top_builddir)/emailrelay.app/Contents/Frameworks
	cp $(srcdir)/Info.plist $(top_builddir)/emailrelay.app/Contents/
	-@touch .bundle

install: .bundle
	cp -Rp $(top_builddir)/emailrelay.app /Applications


