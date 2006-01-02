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

mk_sources=\
	gaddress_ipv4.cpp \
	gclient.cpp \
	gclient_win32.cpp \
	gconnection.cpp \
	gdescriptor_win32.cpp \
	geventhandler.cpp \
	geventloop.cpp \
	geventloop_win32.cpp \
	geventserver.cpp \
	glinebuffer.cpp \
	glocal.cpp \
	glocal_win32.cpp \
	gmonitor.cpp \
	grequest.cpp \
	gresolve.cpp \
	gresolve_ipv4.cpp \
	gresolve_win32.cpp \
	gsender.cpp \
	gserver.cpp \
	gsocket.cpp \
	gsocket_win32.cpp \
	gtimer.cpp

mk_target=gnet.a

all: $(mk_target)

include ../mingw-common.mak

$(mk_target): $(mk_objects)
	$(mk_ar) $(mk_target) $(mk_objects)

