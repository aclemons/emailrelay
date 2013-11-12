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

console_sources=\
	admin_enabled.cpp \
	commandline_full.cpp \
	configuration.cpp \
	legal.cpp \
	news.cpp \
	output.cpp \
	run.cpp

console_objects=$(console_sources:.cpp=.o)

window_sources=\
	winapp.cpp \
	winform.cpp \
	winmain.cpp \
	winmenu.cpp

window_objects=$(window_sources:.cpp=.o)

main_objects=$(console_objects) $(window_objects)
test_objects=$(console_objects) main.o

libs=../gpop/gpop.a ../gsmtp/gsmtp.a ../gnet/gnet.a ../gauth/gauth.a ../gssl/gssl.a ../win32/gwin32.a ../glib/glib.a $(mk_ssl_libs)
glib=../glib/glib.a
syslibs=-lgdi32 -lws2_32 -ladvapi32
rc=emailrelay.rc
fake_mc=fakemc.exe
mc_output=MSG00001.bin messages.rc
mk_exe_main=emailrelay.exe
mk_exe_test=emailrelay-test.exe
mk_exe_filter_copy=emailrelay-filter-copy.exe
mk_exe_poke=emailrelay-poke.exe
mk_exe_passwd=emailrelay-passwd.exe
mk_exe_submit=emailrelay-submit.exe
mk_exe_service=emailrelay-service.exe
res=$(rc:.rc=.o)

all: $(mk_exe_main) $(mk_exe_filter_copy) $(mk_exe_poke) $(mk_exe_passwd) $(mk_exe_submit) $(mk_exe_service) $(mk_exe_test)

include ../mingw-common.mak

$(mk_exe_main): $(main_objects) $(res) $(libs)
	$(mk_link) $(mk_link_flags) -mwindows -o $(mk_exe_main) $(main_objects) $(res) $(libs) $(syslibs)

$(mk_exe_test): $(test_objects) $(res) $(libs)
	$(mk_link) $(mk_link_flags) -o $(mk_exe_test) $(test_objects) $(libs) $(syslibs)

$(mk_exe_filter_copy): filter_copy.o legal.o filter.o
	$(mk_link) $(mk_link_flags) -o $@ filter_copy.o legal.o filter.o $(libs) $(syslibs)

$(mk_exe_poke): poke.o
	$(mk_link) $(mk_link_flags) -o $@ $< $(libs) $(syslibs)

$(mk_exe_passwd): passwd.o legal.o
	$(mk_link) $(mk_link_flags) -o $@ passwd.o legal.o $(libs) $(syslibs)

$(mk_exe_submit): submit.o legal.o
	$(mk_link) $(mk_link_flags) -o $@ submit.o legal.o $(libs) $(syslibs)

$(mk_exe_service): service_install.o service_remove.o service_wrapper.o
	$(mk_link) $(mk_link_flags) -o $@ service_install.o service_remove.o service_wrapper.o $(glib) $(syslibs)

$(fake_mc): fakemc.o
	$(mk_link) $(mk_link_flags) -o $@ $<

$(mc_output): $(fake_mc) messages.mc
	$(fake_mc) messages.mc

$(res): $(rc) $(mc_output)
	$(mk_rc) --include-dir . -i $(rc) -o $@

