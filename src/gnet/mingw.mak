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

mk_sources=\
	gaddress_ipv4.cpp \
	gbufferedserverpeer.cpp \
	gclient.cpp \
	gconnection.cpp \
	gconnectionlookup_win32.cpp \
	gdescriptor.cpp \
	gdescriptor_win32.cpp \
	geventhandler.cpp \
	geventloop.cpp \
	geventloop_win32.cpp \
	gheapclient.cpp \
	glinebuffer.cpp \
	glocal.cpp \
	glocal_win32.cpp \
	gmonitor.cpp \
	gmultiserver.cpp \
	grequest_win32.cpp \
	gresolver.cpp \
	gresolverinfo.cpp \
	gresolver_ipv4.cpp \
	gresolver_win32.cpp \
	gserver.cpp \
	gsimpleclient.cpp \
	gsimpleclient_win32.cpp \
	gsocket.cpp \
	gsocket_win32.cpp \
	gsocketprotocol.cpp \
	gtimer.cpp \
	gtimerlist.cpp

mk_target=gnet.a

all: $(mk_target)

include ../mingw-common.mak

$(mk_target): $(mk_objects)
	$(mk_ar) $(mk_target) $(mk_objects)

