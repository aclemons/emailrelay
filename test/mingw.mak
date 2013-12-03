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

libs=../src/gpop/gpop.a ../src/gsmtp/gsmtp.a ../src/gnet/gnet.a ../src/gauth/gauth.a ../src/gssl/gssl.a ../src/win32/gwin32.a ../src/glib/glib.a $(mk_ssl_libs)
glib=../src/glib/glib.a
syslibs=-lgdi32 -lws2_32 -ladvapi32

all: emailrelay-test-server emailrelay-test-scanner emailrelay-test-verifier

include ../src/mingw-common.mak
mk_src_dir=../src

emailrelay-test-server: emailrelay_test_server.o $(libs)
	$(mk_link) $(mk_link_flags) -o emailrelay-test-server emailrelay_test_server.o $(libs) $(syslibs)

emailrelay-test-scanner: emailrelay_test_scanner.o $(libs)
	$(mk_link) $(mk_link_flags) -o emailrelay-test-scanner emailrelay_test_scanner.o $(libs) $(syslibs)

emailrelay-test-verifier: emailrelay_test_verifier.o $(libs)
	$(mk_link) $(mk_link_flags) -o emailrelay-test-verifier emailrelay_test_verifier.o $(libs) $(syslibs)
