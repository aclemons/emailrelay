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
# mingw.mak
#
# See ../mingw-common.mak for help.
#
# Definitions from mingw-common.mak ...
# - "mk_qt" pointing to the Qt directory (eg. "c:/qt/qt5.1.1/5.1.1/mingw48_32")
# - "mk_mingw" pointing to the MinGW directory
#

.PHONY: all

mk_sources=\
	boot_win32.cpp \
	dir.cpp \
	dir_win32.cpp \
	gdialog.cpp \
	glink_win32.cpp \
	gpage.cpp \
	gregister_win32.cpp \
	guimain.cpp \
	installer.cpp \
	legal.cpp \
	moc_gdialog.cpp \
	moc_gpage.cpp \
	moc_pages.cpp \
	pages.cpp \
	serverconfiguration.cpp

mingw_bin=$(mk_mingw)/bin

service_objects=../main/service_install.o ../main/service_remove.o

gui_syslibs=\
	-lmingw32 \
	-lqwindows \
	-lQt5Widgets \
	-lQt5Gui \
	-lQt5Core \
	-lqtmain

syslibs=\
	-ladvapi32 \
	-lcomdlg32 \
	-lgdi32 \
	-limm32 \
	-lkernel32 \
	-lmsimg32 \
	-lole32 \
	-loleaut32 \
	-lshell32 \
	-luser32 \
	-luuid \
	-lwinspool \
	-lwinmm \
	-lws2_32

defines_static=-DG_QT_STATIC

mk_defines_extra=\
	$(defines_static) \
	-DQT_NO_OPENGL \
	-DQT_LARGEFILE_SUPPORT \
	-DQT_THREAD_SUPPORT \
	-DG_WIN32_IE \
	-DHAVE_ZLIB_H=1

mk_defines_debug_extra=\
	-DQT_DEBUG

mk_defines_release_extra=\
	-DG_WITH_DEBUG \
	-DQT_NO_DEBUG

mk_ccc_flags_extra=\
	-fno-keep-inline-dllexport

mk_link_flags_extra=\
	-Wl,-s \
	-Wl,-subsystem,windows \
	-L$(mk_qt)/lib \
	-L$(mk_qt)/plugins/platforms

mk_link_flags_simple=\
	$(mk_link_flags_common) \
	$(mk_link_flags_release)

glib=../glib/glib.a
libs=$(glib) $(qt_libs_release)

exe_gui=emailrelay-gui.exe
manifest=emailrelay-gui.manifest
make_manifest=../../bin/make-manifest.sh_
make_setup=../../bin/make-setup.sh_
mk_includes_extra=-I$(mk_qt)/include -I$(mk_qt)/include/QtCore -I$(mk_qt)/include/QtGui -I../main

rc=emailrelay-gui.rc
res=emailrelay-gui.o
mc_output=../main/MSG00001.bin ../main/messages.rc

all: $(exe_gui)

include ../mingw-common.mak

$(exe_gui): $(res) $(mk_objects) $(libs) $(service_objects)
	$(mk_link) $(mk_link_flags) -o $(exe_gui) $(res) $(mk_objects) $(service_objects) $(libs) $(gui_syslibs) $(syslibs)

moc_gdialog.cpp: gdialog.h
	$(mk_qt)/bin/moc $< -o $@

moc_gpage.cpp: gpage.h
	$(mk_qt)/bin/moc $< -o $@

moc_pages.cpp: pages.h
	$(mk_qt)/bin/moc $< -o $@

$(res): $(rc) $(mc_output)
	$(make_manifest) highestAvailable > $(manifest)
	$(mk_rc) --include-dir . --include-dir ../main -i $(rc) -o $@

clean::
	$(mk_rm_f) moc_gdialog.cpp moc_gpage.cpp moc_pages.cpp
	$(mk_rm_f) $(exe_gui)
	$(mk_rm_f) *.dll
	$(mk_rm_f) $(res) $(manifest)
	rm -rf dist

payload_cfg="dist/payload/payload.cfg"

.PHONY: setup
setup: ../../doc/userguide.html $(exe_gui)
	-mkdir dist
	-mkdir dist/payload
	$(make_setup) -w dist/payload

../../doc/userguide.html:
	-@echo ..
	-@echo warning: incomplete documentation: add html files from a linux build to the doc directory
	-@echo ..

