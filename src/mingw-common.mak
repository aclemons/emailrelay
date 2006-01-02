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
# mingw-common.mak
#
# Included by "mingw.mak" files.
# 
# (MinGW is the GNU gcc compiler for Windows. It is a free
# alternative to Microsoft Visual Studio. See "http://mingw.org".)
#
# Build with:
#    $ make -f mingw.mak
#
# If building with mingw from cygwin then you must use an 
# explicit path to the mingw tools to avoid picking up the
# cygwin equivalents. Specify the mingw bin directory
# on the make command-line as follows -- note the trailing
# slash:
#   $ make -f mingw.mak mk_bin=c:/opt/mingw/bin/
#

mk_bin=

mk_ar=ar rc
mk_rc=$(mk_bin)windres
mk_rm_f=rm -f
mk_objects=$(mk_sources:.cpp=.o)
mk_cc=$(mk_bin)g++
mk_cc_flags_common=-mno-cygwin -mwindows
mk_cc_flags_release=$(mk_cc_flags_common) -O
mk_cc_flags_debug=$(mk_cc_flags_common) -g
mk_link=$(mk_bin)g++
mk_link_flags_release=--strip-all
mk_link_flags_debug=-g
mk_defines_common=-DG_WIN32 -DG_MINGW
mk_defines_debug=$(mk_defines_common) -D_DEBUG
mk_defines_release=$(mk_defines_common) -DG_NO_DEBUG
mk_includes=-I../glib -I../gnet -I../gsmtp -I../gpop -I../win32
mk_cpp_flags=$(mk_defines) $(mk_defines_extra) $(mk_includes) $(mk_includes_extra)

mk_defines=$(mk_defines_release)
mk_cc_flags=$(mk_cc_flags_release)
mk_link_flags=$(mk_link_flags_release)

.SUFFIXES: .rc .i
.PHONY: clean

.cpp.o:
	$(mk_cc) $(mk_cc_flags) $(mk_cpp_flags) -c $*.cpp

.c.o:
	$(mk_cc) $(mk_cc_flags) $(mk_cpp_flags) -c $*.c

.cpp.i:
	$(mk_cc) $(mk_cc_flags) $(mk_cpp_flags) -E $*.cpp > $*.i

.rc.o:
	$(mk_rc) --include-dir . -i $*.rc -o $*.o

_all:
	cd glib && make -f mingw.mak mk_bin=$(mk_bin) && cd ..
	cd gnet && make -f mingw.mak mk_bin=$(mk_bin) && cd ..
	cd gsmtp && make -f mingw.mak mk_bin=$(mk_bin) && cd ..
	cd gpop && make -f mingw.mak mk_bin=$(mk_bin) && cd ..
	cd win32 && make -f mingw.mak mk_bin=$(mk_bin) && cd ..
	cd main && make -f mingw.mak mk_bin=$(mk_bin) && cd ..

_clean:
	cd glib && make -f mingw.mak mk_bin=$(mk_bin) clean && cd ..
	cd gnet && make -f mingw.mak mk_bin=$(mk_bin) clean && cd ..
	cd gsmtp && make -f mingw.mak mk_bin=$(mk_bin) clean && cd ..
	cd gpop && make -f mingw.mak mk_bin=$(mk_bin) clean && cd ..
	cd win32 && make -f mingw.mak mk_bin=$(mk_bin) clean && cd ..
	cd main && make -f mingw.mak mk_bin=$(mk_bin) clean && cd ..

clean::
	$(mk_rm_f) $(mk_objects) $(mk_target)

