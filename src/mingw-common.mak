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
# mingw-common.mak
#
# Included by "mingw.mak" files.
# 
# MinGW is the GNU gcc compiler for Windows. It is a free alternative to
# Microsoft Visual Studio. It can be obtained from http://mingw.org.
#
# To build using MinGW you should use the MinGW "make" utility called 
# "mingw32-make". The top level makefile "mingw.mak" can be found in the "src" 
# directory. 
#
# For example:
#    c:\emailrelay\src> PATH=c:\mingw\bin;%PATH%
#    c:\emailrelay\src> mingw32-make -f mingw.mak
#
# Edit this makefile to enable TLS/SSL support in the E-MailRelay server
# and to enable the GUI build.
#
# For more information on building the E-MailRelay from source please
# refer to the "doc/developers.txt" document.
#

###
## Edit this section...
##
## "mk_ssl" is set to "openssl" for tls/ssl support, or "none"
mk_ssl=none
##
## "mk_openssl" points to the openssl directory (for openssl build)
mk_openssl=c:/openssl-1.0.1e
##
## "mk_gui" is set to "gui" to enable the GUI build, or "none"
mk_gui=none
##
## "mk_qt" points to the Qt installation directory (for gui build)
mk_qt=c:/qt
##
## "mk_zlib" points to the zlib directory (for gui build)
mk_zlib=$(mk_qt)/src/3rdparty/zlib
##
## "mk_mingw" points to a directory containing the mingw runtime dll (for gui build)
mk_mingw=$(mk_qt)/bin
##
## choose debug or release build ...
mk_includes=$(mk_includes_common) $(mk_includes_release) $(mk_includes_extra)
mk_defines=$(mk_defines_release) $(mk_defines_extra)
mk_cpp_flags=$(mk_defines) $(mk_includes) $(mk_cpp_flags_extra)
mk_cc_flags=$(mk_cc_flags_common) $(mk_cc_flags_release) $(mk_cc_flags_extra)
mk_ccc_flags=$(mk_ccc_flags_common) $(mk_ccc_flags_release) $(mk_ccc_flags_extra)
mk_link_flags=$(mk_link_flags_common) $(mk_link_flags_release) $(mk_link_flags_extra)
##
###

ifeq ("$(mk_ssl)","openssl")
mk_ssl_libs=$(mk_openssl)/libssl.a $(mk_openssl)/libcrypto.a
endif

mk_src_dir=..
mk_ar=ar rc
mk_rc=$(mk_bin)windres
mk_rm_f=rm -f
mk_objects_1=$(mk_sources:.cpp=.o)
mk_objects=$(mk_objects_1:.c=.o)
mk_cc=$(mk_bin)gcc
mk_ccc=$(mk_bin)g++
mk_link=$(mk_bin)g++

mk_ccc_flags_common=-mthreads -frtti -fexceptions
mk_ccc_flags_release=-O $(mk_ccc_flags_release_extra)
mk_ccc_flags_debug=-g $(mk_ccc_flags_debug_extra)
mk_cc_flags_common=-mthreads
mk_cc_flags_release=-O $(mk_cc_flags_release_extra)
mk_cc_flags_debug=-g $(mk_cc_flags_debug_extra)
mk_link_flags_common=-mthreads
mk_link_flags_release=-s -static $(mk_link_flags_release_extra)
mk_link_flags_debug=-g $(mk_link_flags_debug_extra)
mk_defines_common=-DG_WIN32 -DG_MINGW -DUNICODE -D_UNICODE
mk_defines_release=$(mk_defines_common) $(mk_defines_release_extra)
mk_defines_debug=$(mk_defines_common) -D_DEBUG $(mk_defines_debug_extra)
mk_includes_common=-I$(mk_src_dir)/glib -I$(mk_src_dir)/gssl -I$(mk_src_dir)/gnet -I$(mk_src_dir)/gauth -I$(mk_src_dir)/gsmtp -I$(mk_src_dir)/gpop -I$(mk_src_dir)/win32
mk_includes_release=$(mk_includes_release_extra)
mk_includes_debug=$(mk_includes_debug_extra)

.SUFFIXES: .rc .i
.PHONY: clean _clean _all

.cpp.o:
	$(mk_ccc) $(mk_cpp_flags) $(mk_ccc_flags) -c $*.cpp

.c.o:
	$(mk_cc) $(mk_cpp_flags) $(mk_cc_flags) -c $*.c

.cpp.i:
	$(mk_cc) $(mk_cpp_flags) $(mk_cc_flags) -E $*.cpp > $*.i

.rc.o:
	$(mk_rc) --include-dir . -i $*.rc -o $*.o

_all:
	cd glib && $(MAKE) -f mingw.mak mk_bin=$(mk_bin) && cd ..
	cd gssl && $(MAKE) -f mingw.mak mk_bin=$(mk_bin) && cd ..
	cd gnet && $(MAKE) -f mingw.mak mk_bin=$(mk_bin) && cd ..
	cd gauth && $(MAKE) -f mingw.mak mk_bin=$(mk_bin) && cd ..
	cd gsmtp && $(MAKE) -f mingw.mak mk_bin=$(mk_bin) && cd ..
	cd gpop && $(MAKE) -f mingw.mak mk_bin=$(mk_bin) && cd ..
	cd win32 && $(MAKE) -f mingw.mak mk_bin=$(mk_bin) && cd ..
	cd main && $(MAKE) -f mingw.mak mk_bin=$(mk_bin) && cd ..
ifeq ("$(mk_gui)","gui")
	cd gui && $(MAKE) -f mingw.mak mk_bin=$(mk_bin) mk_qt=$(mk_qt) mk_zlib=$(mk_zlib) mk_mingw=$(mk_mingw) && cd ..
endif

_clean:
	cd glib && $(MAKE) -f mingw.mak mk_bin=$(mk_bin) clean && cd ..
	cd gssl && $(MAKE) -f mingw.mak mk_bin=$(mk_bin) clean && cd ..
	cd gnet && $(MAKE) -f mingw.mak mk_bin=$(mk_bin) clean && cd ..
	cd gauth && $(MAKE) -f mingw.mak mk_bin=$(mk_bin) clean && cd ..
	cd gsmtp && $(MAKE) -f mingw.mak mk_bin=$(mk_bin) clean && cd ..
	cd gpop && $(MAKE) -f mingw.mak mk_bin=$(mk_bin) clean && cd ..
	cd win32 && $(MAKE) -f mingw.mak mk_bin=$(mk_bin) clean && cd ..
	cd main && $(MAKE) -f mingw.mak mk_bin=$(mk_bin) clean && cd ..
ifeq ("$(mk_gui)","gui")
	cd gui && $(MAKE) -f mingw.mak mk_bin=$(mk_bin) mk_qt=$(mk_qt) mk_zlib=$(mk_zlib) mk_mingw=$(mk_mingw) clean && cd ..
endif

clean::
	-$(mk_rm_f) $(mk_objects) $(mk_target)

