#
## Copyright (C) 2001-2003 Graeme Walker <graeme_walker@users.sourceforge.net>
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

mk_sources=\
	commandline.cpp \
	configuration.cpp \
	legal.cpp \
	output.cpp \
	run.cpp \
	winapp.cpp \
	winform.cpp \
	winmain.cpp \
	winmenu.cpp

libs=../gsmtp/gsmtp.a ../gnet/gnet.a ../win32/gwin32.a ../glib/glib.a 
syslibs=-lgdi32 -lwsock32
rc=emailrelay.rc
res=$(rc:.rc=.o)

mk_target=emailrelay.exe

all: $(mk_target)

include ../mingw-common.mak

$(mk_target): $(mk_objects) $(res) $(libs)
	$(mk_gcc) $(mk_gcc_flags) -o $(mk_target) $(mk_objects) $(res) $(libs) $(syslibs)


