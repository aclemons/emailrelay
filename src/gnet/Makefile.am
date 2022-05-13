#
## Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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

AM_CPPFLAGS = -I$(top_srcdir)/src/glib -I$(top_srcdir)/src/gssl -I$(top_srcdir)/src/win32

EXTRA_DIST_COMMON=\
	gaddresslocal_unix.cpp \
	gdescriptor_unix.cpp \
	geventloop_select.cpp \
	geventloop_epoll.cpp \
	gfutureevent_unix.cpp \
	gsocket_unix.cpp

if GCONFIG_INTERFACE_NAMES

EXTRA_DIST_INTERFACES = \
	ginterfaces_unix.cpp \
	ginterfaces_none.cpp

INTERFACES_SOURCES = \
	ginterfaces_common.cpp \
	ginterfaces_win32.cpp

else

EXTRA_DIST_INTERFACES = \
	ginterfaces_unix.cpp \
	ginterfaces_common.cpp \
	ginterfaces_win32.cpp

INTERFACES_SOURCES = \
	ginterfaces_none.cpp

endif

libgnet_a_SOURCES = \
	gaddress.h \
	gaddress.cpp \
	gaddress4.cpp \
	gaddress4.h \
	gaddress6.cpp \
	gaddress6.h \
	gaddresslocal_none.cpp \
	gaddresslocal.h \
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
	geventemitter.cpp \
	geventemitter.h \
	geventhandler.cpp \
	geventhandler.h \
	geventloggingcontext.cpp \
	geventloggingcontext.h \
	geventloop.cpp \
	geventloop.h \
	geventloophandles_win32.h \
	geventloophandles_win32.cpp \
	geventloop_win32.cpp \
	geventloop_win32.h \
	gexceptionhandler.cpp \
	gexceptionhandler.h \
	gexceptionsink.cpp \
	gexceptionsink.h \
	gexceptionsource.cpp \
	gexceptionsource.h \
	gfutureevent.h \
	gfutureevent_win32.cpp \
	ginterfaces.h \
	$(INTERFACES_SOURCES) \
	glinebuffer.cpp \
	glinebuffer.h \
	glinestore.cpp \
	glinestore.h \
	glistener.h \
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

AM_CPPFLAGS = -I$(top_srcdir)/src/glib -I$(top_srcdir)/src/gssl

EXTRA_DIST_COMMON=\
	gdescriptor_win32.cpp \
	geventloophandles_win32.cpp \
	geventloophandles_win32.h \
	geventloop_win32.cpp \
	geventloop_win32.h \
	gfutureevent_win32.cpp \
	gsocket_win32.cpp

if GCONFIG_INTERFACE_NAMES

EXTRA_DIST_INTERFACES = \
	ginterfaces_win32.cpp \
	ginterfaces_none.cpp

INTERFACES_SOURCES = \
	ginterfaces_unix.cpp \
	ginterfaces_common.cpp

else

EXTRA_DIST_INTERFACES = \
	ginterfaces_win32.cpp \
	ginterfaces_unix.cpp \
	ginterfaces_common.cpp

INTERFACES_SOURCES = \
	ginterfaces_none.cpp

endif

if GCONFIG_UDS

UDS_SOURCES = \
	gaddresslocal_unix.cpp

EXTRA_DIST_UDS =
	gaddresslocal_none.cpp

else

UDS_SOURCES = \
	gaddresslocal_none.cpp

EXTRA_DIST_UDS =
	gaddresslocal_unix.cpp

endif

if GCONFIG_EPOLL

EXTRA_DIST_EVENTLOOP = \
	geventloop_select.cpp

EVENTLOOP_SOURCES = \
	geventloop_epoll.cpp

else

EXTRA_DIST_EVENTLOOP = \
	geventloop_epoll.cpp

EVENTLOOP_SOURCES = \
	geventloop_select.cpp

endif

EXTRA_DIST = \
	$(EXTRA_DIST_COMMON) \
	$(EXTRA_DIST_INTERFACES) \
	$(EXTRA_DIST_EVENTLOOP) \
	$(EXTRA_DIST_UDS)

libgnet_a_SOURCES = \
	gaddress.cpp \
	gaddress.h \
	gaddress4.h \
	gaddress4.cpp \
	gaddress6.h \
	gaddress6.cpp \
	$(UDS_SOURCES) \
	gaddresslocal.h \
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
	geventemitter.cpp \
	geventemitter.h \
	geventhandler.cpp \
	geventhandler.h \
	geventloggingcontext.cpp \
	geventloggingcontext.h \
	geventloop.cpp \
	geventloop.h \
	$(EVENTLOOP_SOURCES) \
	gexceptionhandler.cpp \
	gexceptionhandler.h \
	gexceptionsink.cpp \
	gexceptionsink.h \
	gexceptionsource.cpp \
	gexceptionsource.h \
	gfutureevent.h \
	gfutureevent_unix.cpp \
	ginterfaces.h \
	$(INTERFACES_SOURCES) \
	glinebuffer.cpp \
	glinebuffer.h \
	glinestore.cpp \
	glinestore.h \
	glistener.h \
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
