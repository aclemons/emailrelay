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
# - "mk_zlib" pointing to the directory containing the zlib library
# - "mk_mingw" pointing to the MinGW directory
#
# External configuration options...
# - set "cfg_dynamic" to "1" for dynamic linking
# - set "cfg_qt_version" to "4" for qt4 (untested)
# - set "cfg_packaging_outer" to "packed", "iexpress" or "none"
# - set "cfg_packaging_inner" to "directory", "packed" or "payload"
#
# Use "make ... setup" to build the release package.
#

.PHONY: all

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

zlib_include=$(mk_zlib)
zlib_lib=$(mk_zlib)/libz.a
mingw_bin=$(mk_mingw)/bin

service_objects=../main/service_install.o ../main/service_remove.o

ifeq ("$(cfg_packaging_outer)","")
cfg_packaging_outer=none
endif

ifeq ("$(cfg_packaging_inner)","")
cfg_packaging_inner=directory
endif

ifeq ("$(mk_qt_version)","4")
gui_syslibs=\
	-lmingw32 \
	-lqtmain \
	-lQtCore4 \
	-lQtGui4
else
ifeq ("$(cfg_dynamic)","1")
gui_syslibs=\
	-lmingw32 \
	-lQt5Widgets \
	-lQt5Gui \
	-lQt5Core \
	-lqtmain
else
gui_syslibs=\
	-lmingw32 \
	-lqwindows \
	-lQt5Widgets \
	-lQt5Gui \
	-lQt5Core \
	-lqtmain
endif
endif

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

ifeq ("$(cfg_dynamic)","1")
defines_static=
else
defines_static=-DG_QT_STATIC
endif

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

out_dir=emailrelay-1.9
exe_gui=emailrelay-gui.exe
exe_gui_packed=emailrelay-gui-tmp.exe
exe_setup_outer=emailrelay-1.9-setup.exe
setup_inner=emailrelay-setup
exe_setup_inner=$(setup_inner).exe
inner_manifest=$(setup_inner).exe.manifest
exe_pack=pack.exe
exe_start=emailrelay-start-gui.exe
mk_includes_extra=-I$(zlib_include) -I$(mk_qt)/include -I$(mk_qt)/include/QtCore -I$(mk_qt)/include/QtGui -I../main

rc=emailrelay-gui.rc
res=emailrelay-gui.o
mc_output=../main/MSG00001.bin ../main/messages.rc

# define extra library dlls to pack if not statically linked
ifeq ("$(cfg_dynamic)","1")
ifeq ("$(cfg_qt_version)","4")
qt_bin_dlls=\
	QtCore4.dll \
	Qt5Gui4.dll \
	mingwm10.dll
qt_plugins_dlls=
pack_dlls_1=$(mk_qt)/bin/QtCore4.dll QtCore4.dll
pack_dlls_2=$(mk_qt)/bin/QtGui4.dll Qt5Gui4.dll
pack_dlls_3=$(mingw_bin)/mingwm10.dll mingwm10.dll
else
qt_bin_dlls=\
	icudt51.dll \
	icuin51.dll \
	icuuc51.dll \
	libgcc_s_dw2-1.dll \
	libstdc++-6.dll \
	libwinpthread-1.dll \
	Qt5Core.dll \
	Qt5Gui.dll \
	Qt5Widgets.dll
qt_plugins_dlls=\
	qwindows.dll
pack_dlls_0=$(mk_qt)/bin/icudt51.dll icudt51.dll
pack_dlls_1=$(mk_qt)/bin/icuin51.dll icuin51.dll
pack_dlls_2=$(mk_qt)/bin/icuuc51.dll icuuc51.dll
pack_dlls_3=$(mk_qt)/bin/libgcc_s_dw2-1.dll libgcc_s_dw2-1.dll
pack_dlls_4=$(mk_qt)/bin/libstdc++-6.dll libstdc++-6.dll
pack_dlls_5=$(mk_qt)/bin/libwinpthread-1.dll libwinpthread-1.dll
pack_dlls_6=$(mk_qt)/bin/Qt5Core.dll Qt5Core.dll
pack_dlls_7=$(mk_qt)/bin/Qt5Gui.dll Qt5Gui.dll
pack_dlls_8=$(mk_qt)/bin/Qt5Widgets.dll Qt5Widgets.dll
pack_dlls_9=$(mk_qt)/plugins/platforms/qwindows.dll qwindows.dll
endif
endif
pack_dlls=\
	$(pack_dlls_0) $(pack_dlls_1) $(pack_dlls_2) $(pack_dlls_3) $(pack_dlls_4) \
	$(pack_dlls_5) $(pack_dlls_6) $(pack_dlls_7) $(pack_dlls_8) $(pack_dlls_9)

all: $(exe_gui) $(exe_pack) 

.PHONY: setup
setup: $(exe_setup_outer)

include ../mingw-common.mak

# outer packaging = "none" 
#
# The outer package is a simple copy of the inner gui, which should 
# be packed and statically-linked.
#
ifeq ("$(cfg_packaging_outer)","none")
ifeq ("$(cfg_packaging_inner)","directory")
$(exe_setup_outer): $(out_dir)/readme.txt
	-@echo ..
	-@echo .. now zip up the $(out_dir) directory into emailrelay-1.9.zip
	-@echo .. right click on the directory and send-to compressed folder
	-@echo ..
else
$(exe_setup_outer): $(exe_gui_packed)
	cp $(exe_gui_packed) $(exe_setup_outer)
endif
endif

# outer packaging = "packed"
#
# The outer package is the inner packed gui, or the unpacked gui 
# and its payload file, together with runtime dlls.
#
ifeq ("$(cfg_packaging_outer)","packed")
ifeq ("$(cfg_packaging_inner)","packed")
$(exe_setup_outer): $(exe_pack) $(exe_gui_packed) $(exe_start)
	./$(exe_pack) $(exe_setup_outer) $(exe_start) $(pack_dlls) $(exe_gui_packed) $(exe_gui)
else
$(exe_setup_outer): $(exe_pack) $(exe_gui) payload $(exe_start)
	./$(exe_pack) $(exe_setup_outer) $(exe_start) $(pack_dlls) $(exe_gui) $(exe_gui) payload payload
endif
endif

# outer packaging = "iexpress"
#
# The outer package is the self-extracting executable containing the
# inner packed gui, or the unpacked gui and its payload file, together
# with runtime dlls.
#
ifeq ("$(cfg_packaging_outer)","iexpress")
ifeq ("$(cfg_packaging_inner)","packed")
$(exe_setup_outer): $(exe_pack) $(exe_gui_packed)
	cp $(exe_gui_packed) $(exe_setup_inner)
	./$(exe_pack) -x $(exe_setup_outer) $(exe_setup_inner) $(pack_dlls) $(exe_setup_inner) $(exe_setup_inner) $(inner_manifest) $(inner_manifest)
else
$(exe_setup_outer): $(exe_pack) $(exe_gui) payload $(inner_manifest)
	cp $(exe_gui) $(exe_setup_inner)
	./$(exe_pack) -x $(exe_setup_outer) $(exe_setup_inner) $(pack_dlls) $(exe_setup_inner) $(exe_setup_inner) payload payload $(inner_manifest) $(inner_manifest)
endif
endif

.PHONY: manifest
manifest: $(inner_manifest)

$(inner_manifest): mingw.mak
	echo "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\" ?>" > $(inner_manifest)
	echo "<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">" >> $(inner_manifest)
	echo "<assemblyIdentity version=\"1.0.0.0\" processorArchitecture=\"X86\" name=\"$(setup_inner)\" type=\"win32\" />" >> $(inner_manifest)
	echo "<description>E-MailRelay setup</description>" >> $(inner_manifest)
	echo "<trustInfo xmlns=\"urn:schemas-microsoft-com:asm.v2\">" >> $(inner_manifest)
	echo "<security>" >> $(inner_manifest)
	echo "<requestedPrivileges>" >> $(inner_manifest)
	echo "<requestedExecutionLevel level=\"requireAdministrator\" uiAccess=\"false\" />" >> $(inner_manifest)
	echo "</requestedPrivileges>" >> $(inner_manifest)
	echo "</security>" >> $(inner_manifest)
	echo "</trustInfo>" >> $(inner_manifest)
	echo "</assembly>" >> $(inner_manifest)
	unix2dos $(inner_manifest)

$(exe_gui): $(res) $(mk_objects) $(libs) $(service_objects)
	$(mk_link) $(mk_link_flags) -o $(exe_gui) $(res) $(mk_objects) $(service_objects) $(libs) $(gui_syslibs) $(syslibs) $(zlib_lib)

$(exe_pack): pack.o
	$(mk_link) $(mk_link_flags_simple) -o $(exe_pack) pack.o $(glib) $(zlib_lib)

$(exe_start): guistart.o unpack.o
	$(mk_link) $(mk_link_flags_simple) -o $(exe_start) guistart.o unpack.o $(zlib_lib)

css=../../doc/emailrelay.css
$(css): ../../doc/emailrelay.css_
	cp ../../doc/emailrelay.css_ ../../doc/emailrelay.css

examples=../../bin/emailrelay-edit-content.js ../../bin/emailrelay-edit-envelope.js ../../bin/emailrelay-resubmit.js ../../bin/emailrelay-runperl.js

../../doc/userguide.html:
	-@echo ..
	-@echo warning: incomplete documentation: add html files from a linux build to the doc directory
	-@echo ..

# inner packaging = "packed"
#
# Create a packed gui executable.
#
$(exe_gui_packed): $(exe_gui) $(exe_pack) $(css) ../../doc/userguide.html
	./$(exe_pack) -a $(exe_gui_packed) $(exe_gui) ../../README readme.txt ../../COPYING copying.txt ../../ChangeLog changelog.txt ../../AUTHORS authors.txt ../../doc/doxygen-missing.html doc/doxygen/index.html --dir "" ../main/emailrelay-service.exe ../main/emailrelay.exe ../main/emailrelay-submit.exe ../main/emailrelay-filter-copy.exe ../main/emailrelay-poke.exe ../main/emailrelay-passwd.exe --dir examples $(examples) --dir "doc" ../../doc/*.png ../../doc/*.txt $(css) --opt ../../doc/*.html

# inner packaging = "payload"
#
# Pack into a payload file.
#
payload: $(css) ../../doc/userguide.html
	./$(exe_pack) -a payload NONE ../../README readme.txt ../../COPYING copying.txt ../../ChangeLog changelog.txt ../../AUTHORS authors.txt ../../doc/doxygen-missing.html doc/doxygen/index.html --dir "" ../main/emailrelay-service.exe ../main/emailrelay.exe ../main/emailrelay-submit.exe ../main/emailrelay-filter-copy.exe ../main/emailrelay-poke.exe ../main/emailrelay-passwd.exe --dir examples $(examples) --dir "doc" ../../doc/*.png ../../doc/*.txt $(css) --opt ../../doc/*.html

# inner packaging = "directory"
#
# Pack into the <out-dir> directory. Include a payload file for the unpacked gui
# to work with. Change the name of the gui to "emailrelay-setup.exe" and include
# a manifest for UAC.
#
$(out_dir)/readme.txt: $(css) ../../doc/userguide.html payload $(exe_gui) $(inner_manifest)
	./$(exe_pack) -p -a -d $(out_dir) NONE payload payload $(exe_gui) emailrelay-setup.exe $(inner_manifest) $(inner_manifest) ../../README readme.txt ../../doc/windows.txt readme-windows.txt ../../COPYING copying.txt ../../ChangeLog changelog.txt ../../AUTHORS authors.txt ../../doc/doxygen-missing.html doc/doxygen/index.html --dir "" ../main/emailrelay-service.exe ../main/emailrelay.exe ../main/emailrelay-submit.exe ../main/emailrelay-filter-copy.exe ../main/emailrelay-poke.exe ../main/emailrelay-passwd.exe --dir examples $(examples) --dir "doc" ../../doc/*.png ../../doc/*.txt $(css) --opt ../../doc/*.html

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
	$(mk_rm_f) $(exe_gui) $(exe_pack) $(exe_start) $(exe_gui_packed) $(exe_setup_outer) $(exe_setup_inner)
	$(mk_rm_f) *.dll
	$(mk_rm_f) $(res)

