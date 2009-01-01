#
## Copyright (C) 2001-2009 Graeme Walker <graeme_walker@users.sourceforge.net>
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
# - "mk_qt" pointing to the Qt4 directory
# - "mk_zlib" pointing to the directory containing the zlib library (optional)
# - "mk_mingw" pointing to the MinGW bin directory (optional)
#

.PHONY: all

ifeq ("$(mk_qt)","")
mk_qt=c:/qt
endif

ifeq ("$(mk_mingw)","")
mk_mingw=$(mk_qt)/bin
endif

ifeq ("$(mk_zlib)","")
mk_zlib=$(mk_qt)/src/3rdparty/zlib
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
	moc_gdialog.cpp \
	moc_gpage.cpp \
	moc_pages.cpp \
	glink_win32.cpp \
	gunpack.cpp \
	state.cpp \
	unpack.c

service_objects=../main/service_install.o ../main/service_remove.o

gui_syslibs=\
	-lqtmain \
	-lQtCore4 \
	-lQtGui4 \
	-ladvapi32 \
	-lcomdlg32 \
	-lgdi32 \
	-limm32 \
	-lkernel32 \
	-lmingw32 \
	-lmsimg32 \
	-lole32 \
	-loleaut32 \
	-lshell32 \
	-luser32 \
	-luuid \
	-lwinmm \
	-lwinspool \
	-lws2_32

syslibs=\
	-lkernel32 \
	-luser32 \
	-lgdi32 \
	-lole32 \
	-loleaut32 \
	-luuid

mk_defines_extra=\
	-DG_WIN32_IE \
	-DG_WIN32_DCOM \
	-DQT_LARGEFILE_SUPPORT \
	-DQT_GUI_LIB \
	-DQT_CORE_LIB \
	-DQT_THREAD_SUPPORT \
	-DQT_NEEDS_QMAIN \
	-DHAVE_ZLIB_H=1

mk_defines_release_extra=\
	-DQT_NO_DEBUG

mk_ccc_flags_extra=\
	-frtti \
	-fexceptions

mk_link_flags_extra=\
	-L$(mk_qt)/lib \
	-mthreads \
	-Wl,-enable-stdcall-fixup \
	-Wl,-enable-auto-import \
	-Wl,-enable-runtime-pseudo-reloc \
	-static \
	-Wl,-s \
	-Wl,-subsystem,windows

mk_link_flags_simple=\
	$(mk_link_flags_common) \
	$(mk_link_flags_release)

zlib=$(mk_zlib)/libz.a
glib=../glib/glib.a
libs=$(glib) $(qt_libs_release)

mk_exe_gui=emailrelay-gui.exe
mk_exe_gui_tmp=emailrelay-gui-tmp.exe
mk_exe_setup=emailrelay-setup.exe
mk_exe_pack=pack.exe
mk_exe_run=run.exe
mk_includes_extra=-I$(mk_qt)/include -I$(mk_qt)/include/QtCore -I$(mk_qt)/include/QtGui -I$(mk_zlib) -I../main
mk_dll_qt_1=QtCore4.dll
mk_dll_qt_2=QtGui4.dll
mk_dll_mingw=mingwm10.dll

all: $(mk_exe_gui) $(mk_exe_pack) $(mk_exe_setup)

include ../mingw-common.mak

$(mk_exe_gui): $(mk_objects) $(libs) $(service_objects)
	$(mk_link) $(mk_link_flags) -o $(mk_exe_gui) $(mk_objects) $(service_objects) $(libs) $(gui_syslibs) $(gui_syslibs)

$(mk_exe_pack): pack.o
	$(mk_link) $(mk_link_flags_simple) -o $(mk_exe_pack) pack.o $(glib) $(zlib)

$(mk_exe_run): run.o unpack.o $(zlib)
	$(mk_link) $(mk_link_flags_simple) -o $(mk_exe_run) run.o unpack.o $(zlib)

$(mk_exe_gui_tmp): $(mk_exe_gui) $(mk_exe_pack)
	./$(mk_exe_pack) -a $(mk_exe_gui_tmp) $(mk_exe_gui) ../../README readme.txt ../../COPYING copying.txt ../../ChangeLog changelog.txt ../../AUTHORS authors.txt ../../doc/doxygen_missing.html doc/doxygen/index.html --dir "" ../main/emailrelay-service.exe ../main/emailrelay.exe ../main/emailrelay-submit.exe ../main/emailrelay-filter-copy.exe ../main/emailrelay-poke.exe ../main/emailrelay-passwd.exe --dir "doc" ../../doc/*.png ../../doc/*.txt ../../doc/emailrelay.css --opt ../../doc/*.html

../../doc/userguide.html:
	-@echo ..
	-@echo warning: incomplete html documentation in the setup exe: add to doc directory
	-@echo ..

$(mk_exe_setup): $(mk_exe_pack) $(mk_exe_gui_tmp) $(mk_exe_run) ../../doc/userguide.html
	./$(mk_exe_pack) $(mk_exe_setup) $(mk_exe_run) $(mk_qt)/bin/$(mk_dll_qt_1) $(mk_dll_qt_1) $(mk_qt)/bin/$(mk_dll_qt_2) $(mk_dll_qt_2) $(mk_mingw)/$(mk_dll_mingw) $(mk_dll_mingw) $(mk_exe_gui_tmp) $(mk_exe_gui)

moc_gdialog.cpp: gdialog.h
	$(mk_qt)/bin/moc $< -o $@

moc_gpage.cpp: gpage.h
	$(mk_qt)/bin/moc $< -o $@

moc_pages.cpp: pages.h
	$(mk_qt)/bin/moc $< -o $@

clean::
	$(mk_rm_f) moc_gdialog.cpp moc_gpage.cpp moc_pages.cpp
	$(mk_rm_f) $(mk_exe_gui) $(mk_exe_pack) $(mk_exe_run) $(mk_exe_gui_tmp) $(mk_exe_setup) 
	$(mk_rm_f) $(mk_dll_qt_1) $(mk_dll_qt_2) $(mk_dll_mingw)

