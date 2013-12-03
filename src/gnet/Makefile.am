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

if DNS
DNS_EXTRA_DIST = gresolver_dns_disabled.cpp glocal_dns_disabled.cpp
DNS_SOURCES = gresolver_unix.cpp gresolver_ip.cpp glocal_unix.cpp
else
DNS_EXTRA_DIST = gresolver_unix.cpp gresolver_ip.cpp glocal_unix.cpp
DNS_SOURCES = gresolver_dns_disabled.cpp glocal_dns_disabled.cpp
endif

EXTRA_DIST=\
	$(DNS_EXTRA_DIST) \
	gaddress_ipv4.cpp \
	gaddress_ipv6.cpp \
	gconnectionlookup_win32.cpp \
    gdescriptor_win32.cpp \
    geventloop_win32.cpp \
    glocal_win32.cpp \
	gresolver_ipv4.cpp \
	gresolver_ipv6.cpp \
    grequest_win32.cpp \
    gresolver_win32.cpp \
    gsimpleclient_win32.cpp \
    gsocket_win32.cpp \
    mingw.mak

INCLUDES = -I$(top_srcdir)/lib/$(COMPILER_VERSION) -I$(top_srcdir)/src/glib  -I$(top_srcdir)/src/gssl
noinst_LIBRARIES = libgnet.a

libgnet_a_SOURCES = \
	$(DNS_SOURCES) \
	gmonitor.cpp \
	gaddress_ip.cpp \
	gaddress.h \
	gbufferedserverpeer.cpp \
	gbufferedserverpeer.h \
	gclientptr.h \
	gclient.cpp \
	gclient.h \
	gconnection.cpp \
	gconnection.h \
	gconnectionlookup.h \
	gconnectionlookup_unix.cpp \
	gdescriptor.cpp \
	gdescriptor.h \
	gdescriptor_unix.cpp \
	gevent.h \
	geventhandler.cpp \
	geventhandler.h \
	geventloop.cpp \
	geventloop.h \
	geventloop_unix.cpp \
	gheapclient.cpp \
	gheapclient.h \
	glinebuffer.cpp \
	glinebuffer.h \
	glocal.cpp \
	glocal.h \
	gmonitor.h \
	gmultiserver.cpp \
	gmultiserver.h \
	gnet.h \
	grequest.h \
	gresolver.cpp \
	gresolver.h \
	gresolverinfo.cpp \
	gresolverinfo.h \
	gserver.cpp \
	gserver.h \
	gsimpleclient.cpp \
	gsimpleclient.h \
	gsimpleclient_unix.cpp \
	gsocket.h \
	gsocket.cpp \
	gsocket_unix.cpp \
	gsocketprotocol.cpp \
	gsocketprotocol.h \
	gtimer.cpp \
	gtimer.h \
	gtimerlist.cpp \
	gtimerlist.h

