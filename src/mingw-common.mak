#
## Copyright (C) 2001-2004 Graeme Walker <graeme_walker@users.sourceforge.net>
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

mk_bin=/mingw/bin/

mk_ar=ar rc
mk_rc=$(mk_bin)windres
mk_rm_f=rm -f
mk_objects=$(mk_sources:.cpp=.o)
mk_gcc=$(mk_bin)g++
mk_gcc_flags=-mno-cygwin -g -mwindows
mk_defines=-DG_WIN32 -DG_MINGW
mk_includes=-I../glib -I../gnet -I../gsmtp -I../win32
mk_cpp_flags=$(mk_defines) $(mk_includes)

.SUFFIXES: .rc .i

.cpp.o:
	$(mk_gcc) $(mk_gcc_flags) $(mk_cpp_flags) -c $*.cpp

.cpp.i:
	$(mk_gcc) $(mk_gcc_flags) $(mk_cpp_flags) -E $*.cpp > $*.i

.rc.o:
	$(mk_rc) --include-dir . -i $*.rc -o $*.o

_all:
	cd glib && make -f mingw.mak && cd ..
	cd gnet && make -f mingw.mak && cd ..
	cd gsmtp && make -f mingw.mak && cd ..
	cd win32 && make -f mingw.mak && cd ..
	cd main && make -f mingw.mak && cd ..

_clean:
	cd glib && make -f mingw.mak clean && cd ..
	cd gnet && make -f mingw.mak clean && cd ..
	cd gsmtp && make -f mingw.mak clean && cd ..
	cd win32 && make -f mingw.mak clean && cd ..
	cd main && make -f mingw.mak clean && cd ..

clean::
	$(mk_rm_f) $(mk_objects) $(mk_target)

