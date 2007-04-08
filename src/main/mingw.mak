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
# mingw.mak
#
# See ../mingw-common.mak for help.
#

mk_sources=\
	commandline.cpp \
	configuration.cpp \
	legal.cpp \
	news.cpp \
	output.cpp \
	run.cpp \
	winapp.cpp \
	winform.cpp \
	winmain.cpp \
	winmenu.cpp

libs=../gpop/gpop.a ../gsmtp/gsmtp.a ../gnet/gnet.a ../win32/gwin32.a ../glib/glib.a 
syslibs=-lgdi32 -lwsock32
rc=emailrelay.rc
fake_mc=mingw32-mc.exe
mc_output=MSG00001.bin messages.rc
mk_exe_main=emailrelay.exe
mk_exe_filter=emailrelay-filter-copy.exe
mk_exe_poke=emailrelay-poke.exe
mk_exe_passwd=emailrelay-passwd.exe
mk_exe_submit=emailrelay-submit.exe
res=$(rc:.rc=.o)

all: $(mk_exe_main) $(mk_exe_filter) $(mk_exe_poke) $(mk_exe_passwd) $(mk_exe_submit)

include ../mingw-common.mak

$(mk_exe_main): $(mk_objects) $(res) $(libs)
	$(mk_link) $(mk_link_flags) -o $(mk_exe_main) $(mk_objects) $(res) $(libs) $(syslibs)

$(mk_exe_filter): filter_copy.o
	$(mk_link) $(mk_link_flags) -o $@ $< $(libs) $(syslibs)

$(mk_exe_poke): poke.o
	$(mk_link) $(mk_link_flags) -o $@ $< $(libs) $(syslibs)

$(mk_exe_passwd): passwd.o legal.o
	$(mk_link) $(mk_link_flags) -o $@ passwd.o legal.o $(libs) $(syslibs)

$(mk_exe_submit): submit.o legal.o
	$(mk_link) $(mk_link_flags) -o $@ submit.o legal.o $(libs) $(syslibs)

$(fake_mc): mingw.o
	$(mk_link) $(mk_link_flags) -o $@ $<

$(mc_output): $(fake_mc) messages.mc
	$(fake_mc) messages.mc

$(res): $(rc) $(mc_output)

