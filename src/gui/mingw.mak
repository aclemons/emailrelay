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

mk_os=xp

mk_sources=\
	gdialog.cpp \
	gpage.cpp \
	dir.cpp \
	dir_win32.cpp \
	dir_win$(mk_os).cpp \
	main.cpp \
	legal.cpp \
	pages.cpp \
	thread.cpp \
	moc_gdialog.cpp \
	moc_gpage.cpp \
	moc_pages.cpp \
	moc_thread.cpp \
	unpack.c

gui_syslibs=\
	-lQtCore \
	-lQtGui \
	-lqtmain \
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
	-luuid \

mk_defines_extra=\
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

zlib=$(mk_zlib)/libz.a
glib=../glib/glib.a
libs=$(glib) $(qt_libs_release)

mk_target=emailrelay-gui.exe
mk_setup=emailrelay-setup.exe
mk_install_tool=emailrelay-install-tool.exe
mk_pack=pack.exe
mk_run=run.exe
mk_includes_extra=-I$(mk_qt)/include -I$(mk_qt)/include/QtCore -I$(mk_qt)/include/QtGui -I$(mk_zlib)

all: $(mk_target) $(mk_install_tool) $(mk_pack) $(mk_setup)

include ../mingw-common.mak

$(mk_target): $(mk_objects) $(libs)
	$(mk_link) $(mk_link_flags) -o $(mk_target) $(mk_objects) $(libs) $(gui_syslibs) $(gui_syslibs)

$(mk_install_tool): tool.o glink.o $(libs) $(mk_pack)
	$(mk_link) $(mk_link_flags) -o tool.exe tool.o glink.o $(libs) $(syslibs)
	./$(mk_pack) $(mk_install_tool) tool.exe ../main/emailrelay.exe emailrelay.exe ../main/emailrelay-passwd.exe emailrelay-passwd.exe ../doc/readme.txt readme.txt

$(mk_run): run.o unpack.o
	$(mk_cc) -o $(mk_run) run.o unpack.o $(zlib)

$(mk_pack): pack.o
	$(mk_link) $(mk_link_flags) -o $(mk_pack) pack.o $(glib) $(zlib)

$(mk_setup): $(mk_pack) $(mk_target) $(mk_install_tool) $(mk_run)
	./$(mk_pack) $(mk_setup) $(mk_run) $(mk_target) $(mk_target) $(mk_install_tool) $(mk_install_tool)

moc_gdialog.cpp: gdialog.h
	$(mk_qt)/bin/moc $< -o $@

moc_gpage.cpp: gpage.h
	$(mk_qt)/bin/moc $< -o $@

moc_pages.cpp: pages.h
	$(mk_qt)/bin/moc $< -o $@

moc_thread.cpp: thread.h
	$(mk_qt)/bin/moc $< -o $@

