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
# External definitions ...
# - "mk_qt" pointing to the Qt5 directory (eg. "c:/qt/qt5.1.1/5.1.1/mingw48_32")
# - "mk_zlib" pointing to the directory containing the zlib library
# - "mk_mingw" pointing to the MinGW bin directory
#

.PHONY: all

ifeq ("$(mk_qt)","")
mk_qt=c:/qt
endif

ifeq ("$(mk_zlib)","")
zlib_include=/c/zlib
zlib_lib=/c/zlib/libz.a
else
zlib_include=$(mk_zlib)
zlib_lib=$(mk_zlib)/libz.a
endif

ifeq ("$(mk_mingw)","")
mk_mingw_bin=c:/mingw/bin
else
mk_mingw_bin=$(mk_mingw)/bin
endif

mk_sources=\
	boot_win32.cpp \
	gdialog.cpp \
	gpage.cpp \
	dir.cpp \
	dir_win32.cpp \
	guimain.cpp \
	installer.cpp \
	legal.cpp \
	pages.cpp \
	pointer.cpp \
	mapfile.cpp \
	moc_gdialog.cpp \
	moc_gpage.cpp \
	moc_pages.cpp \
	glink_win32.cpp \
	gregister_win32.cpp \
	gunpack.cpp \
	state.cpp \
	unpack.c

service_objects=../main/service_install.o ../main/service_remove.o

gui_syslibs=\
	-lmingw32 \
	-lqtmain \
	-lQt5Gui \
	-lQt5Widgets \
	-lQt5Core

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

mk_defines_extra=\
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
	-L$(mk_qt)/lib

mk_link_flags_simple=\
	$(mk_link_flags_common) \
	$(mk_link_flags_release)

glib=../glib/glib.a
libs=$(glib) $(qt_libs_release)

mk_exe_gui=emailrelay-gui.exe
mk_exe_gui_tmp=emailrelay-gui-tmp.exe
mk_exe_setup=emailrelay-setup.exe
mk_exe_pack=pack.exe
mk_exe_run=run.exe
mk_includes_extra=-I$(zlib_include) -I$(mk_qt)/include -I$(mk_qt)/include/QtCore -I$(mk_qt)/include/QtGui -I../main

# runtime library dlls
mk_dll_qt_1=Qt5Core.dll
mk_dll_qt_2=Qt5Gui.dll
mk_dll_mingw_1=mingwm10.dll
mk_pack_dlls=\
	$(mk_qt)/bin/$(mk_dll_qt_1) $(mk_dll_qt_1) \
	$(mk_qt)/bin/$(mk_dll_qt_2) $(mk_dll_qt_2) \
	$(mk_mingw_bin)/$(mk_dll_mingw_1) $(mk_dll_mingw_1)

rc=emailrelay-gui.rc
res=emailrelay-gui.o
mc_output=../main/MSG00001.bin ../main/messages.rc

all: $(mk_exe_gui) $(mk_exe_pack) $(mk_exe_setup)

include ../mingw-common.mak

$(mk_exe_gui): $(res) $(mk_objects) $(libs) $(service_objects)
	$(mk_link) $(mk_link_flags) -o $(mk_exe_gui) $(res) $(mk_objects) $(service_objects) $(libs) $(gui_syslibs) $(syslibs) $(zlib_lib)

$(mk_exe_pack): pack.o
	$(mk_link) $(mk_link_flags_simple) -o $(mk_exe_pack) pack.o $(glib) $(zlib_lib)

$(mk_exe_run): run.o unpack.o
	$(mk_link) $(mk_link_flags_simple) -o $(mk_exe_run) run.o unpack.o $(zlib_lib)

css=../../doc/emailrelay.css
$(css): ../../doc/emailrelay.css_
	cp ../../doc/emailrelay.css_ ../../doc/emailrelay.css

examples=../../bin/emailrelay-edit-content.js ../../bin/emailrelay-edit-envelope.js ../../bin/emailrelay-resubmit.js ../../bin/emailrelay-runperl.js

$(mk_exe_gui_tmp): $(mk_exe_gui) $(mk_exe_pack) $(css)
	./$(mk_exe_pack) -a $(mk_exe_gui_tmp) $(mk_exe_gui) ../../README readme.txt ../../COPYING copying.txt ../../ChangeLog changelog.txt ../../AUTHORS authors.txt ../../doc/doxygen-missing.html doc/doxygen/index.html --dir "" ../main/emailrelay-service.exe ../main/emailrelay.exe ../main/emailrelay-submit.exe ../main/emailrelay-filter-copy.exe ../main/emailrelay-poke.exe ../main/emailrelay-passwd.exe --dir examples $(examples) --dir "doc" ../../doc/*.png ../../doc/*.txt $(css) --opt ../../doc/*.html

../../doc/userguide.html:
	-@echo ..
	-@echo warning: incomplete html documentation in the setup exe: add to doc directory
	-@echo ..

## double-pack if necessary to include the runtime library dlls - not necessary if statically linked
##$(mk_exe_setup): $(mk_exe_pack) $(mk_exe_gui_tmp) $(mk_exe_run) ../../doc/userguide.html
##	./$(mk_exe_pack) $(mk_exe_setup) $(mk_exe_run) $(mk_pack_dlls) $(mk_exe_gui_tmp) $(mk_exe_gui)
$(mk_exe_setup): $(mk_exe_gui_tmp)
	cp $(mk_exe_gui_tmp) $(mk_exe_setup)

moc_gdialog.cpp: gdialog.h
	$(mk_qt)/bin/moc $< -o $@

moc_gpage.cpp: gpage.h
	$(mk_qt)/bin/moc $< -o $@

moc_pages.cpp: pages.h
	$(mk_qt)/bin/moc $< -o $@

$(res): $(rc) $(mc_output)
	$(mk_rc) --include-dir . --include-dir ../main -i $(rc) -o $@

clean::
	$(mk_rm_f) moc_gdialog.cpp moc_gpage.cpp moc_pages.cpp
	$(mk_rm_f) $(mk_exe_gui) $(mk_exe_pack) $(mk_exe_run) $(mk_exe_gui_tmp) $(mk_exe_setup) 
	$(mk_rm_f) $(mk_dll_qt_1) $(mk_dll_qt_2) $(mk_dll_mingw)
	$(mk_rm_f) $(res)

