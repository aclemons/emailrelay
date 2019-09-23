#
## Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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

noinst_LIBRARIES = libgnet.a

if GCONFIG_WINDOWS

AM_CPPFLAGS = -I$(top_srcdir)/src/glib  -I$(top_srcdir)/src/gssl -I$(top_srcdir)/src/win32

EXTRA_DIST=\
	gaddress_ipv4.cpp \
	gaddress_ipv6.cpp \
	gdescriptor_unix.cpp \
	geventloop_unix.cpp \
	gfutureevent_unix.cpp \
	gsocket_unix.cpp

libgnet_a_SOURCES = \
	gaddress4.cpp \
	gaddress4.h \
	gaddress6.cpp \
	gaddress6.h \
	gaddress.h \
	gaddress_ip.cpp \
	gclient.cpp \
	gclient.h \
	gclientptr.cpp \
	gclientptr.h \
	gconnection.cpp \
	gconnection.h \
	gdescriptor.h \
	gdescriptor_win32.cpp \
	gdnsblock.cpp \
	gdnsblock.h \
	gdnsmessage.cpp \
	gdnsmessage.h \
	gdnsmessage.cpp \
	gdnsmessage.h \
	gevent.h \
	geventhandler.cpp \
	geventhandler.h \
	geventhandlerlist.cpp \
	geventhandlerlist.h \
	geventloop.cpp \
	geventloop.h \
	geventloop_win32.cpp \
	gexceptionhandler.cpp \
	gexceptionhandler.h \
	gexceptionsink.cpp \
	gexceptionsink.h \
	gexceptionsource.cpp \
	gexceptionsource.h \
	gfutureevent.h \
	gfutureevent_win32.cpp \
	glinebuffer.cpp \
	glinebuffer.h \
	glinestore.cpp \
	glinestore.h \
	glocal.cpp \
	glocal.h \
	glocation.cpp \
	glocation.h \
	gmonitor.cpp \
	gmonitor.h \
	gmultiserver.cpp \
	gmultiserver.h \
	gnetdone.cpp \
	gnetdone.h \
	gresolver.cpp \
	gresolver.h \
	gresolverfuture.cpp \
	gresolverfuture.h \
	gserver.cpp \
	gserver.h \
	gserverpeer.cpp \
	gserverpeer.h \
	gsocket.cpp \
	gsocket.h \
	gsocketprotocol.cpp \
	gsocketprotocol.h \
	gsocket_win32.cpp \
	gsocks.cpp \
	gsocks.h \
	gtask.cpp \
	gtask.h \
	gtimer.cpp \
	gtimer.h \
	gtimerlist.cpp \
	gtimerlist.h

else

AM_CPPFLAGS = -I$(top_srcdir)/src/glib  -I$(top_srcdir)/src/gssl

EXTRA_DIST=\
	gaddress_ipv4.cpp \
	gaddress_ipv6.cpp \
    gdescriptor_win32.cpp \
    geventloop_win32.cpp \
	gfutureevent_win32.cpp \
    gsocket_win32.cpp

libgnet_a_SOURCES = \
	gaddress_ip.cpp \
	gaddress.h \
	gaddress4.h \
	gaddress4.cpp \
	gaddress6.h \
	gaddress6.cpp \
	gclient.cpp \
	gclient.h \
	gclientptr.cpp \
	gclientptr.h \
	gconnection.cpp \
	gconnection.h \
	gdescriptor.h \
	gdescriptor_unix.cpp \
	gdnsblock.cpp \
	gdnsblock.h \
	gdnsmessage.cpp \
	gdnsmessage.h \
	gevent.h \
	geventhandler.cpp \
	geventhandler.h \
	geventhandlerlist.cpp \
	geventhandlerlist.h \
	geventloop.cpp \
	geventloop.h \
	geventloop_unix.cpp \
	gexceptionhandler.cpp \
	gexceptionhandler.h \
	gexceptionsink.cpp \
	gexceptionsink.h \
	gexceptionsource.cpp \
	gexceptionsource.h \
	gfutureevent.h \
	gfutureevent_unix.cpp \
	glinebuffer.cpp \
	glinebuffer.h \
	glinestore.cpp \
	glinestore.h \
	glocal.cpp \
	glocal.h \
	glocation.cpp \
	glocation.h \
	gmonitor.cpp \
	gmonitor.h \
	gmultiserver.cpp \
	gmultiserver.h \
	gnetdone.cpp \
	gnetdone.h \
	gresolver.cpp \
	gresolver.h \
	gresolverfuture.cpp \
	gresolverfuture.h \
	gserver.cpp \
	gserver.h \
	gserverpeer.cpp \
	gserverpeer.h \
	gsocket.h \
	gsocket.cpp \
	gsocket_unix.cpp \
	gsocketprotocol.cpp \
	gsocketprotocol.h \
	gsocks.cpp \
	gsocks.h \
	gtask.cpp \
	gtask.h \
	gtimer.cpp \
	gtimer.h \
	gtimerlist.cpp \
	gtimerlist.h

endif
