#
## Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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

EXTRA_LIBRARIES = libgnetextra.a

COMMON_SOURCES = \
	gaddress.cpp \
	gaddress.h \
	gaddress4.h \
	gaddress4.cpp \
	gaddress6.h \
	gaddress6.cpp \
	gaddresslocal.h \
	gclient.cpp \
	gclient.h \
	gclientptr.cpp \
	gclientptr.h \
	gconnection.cpp \
	gconnection.h \
	gdescriptor.h \
	gdnsmessage.h \
	gdnsmessage.cpp \
	gevent.h \
	geventemitter.cpp \
	geventemitter.h \
	geventhandler.cpp \
	geventhandler.h \
	geventloggingcontext.cpp \
	geventloggingcontext.h \
	geventloop.cpp \
	geventloop.h \
	gexceptionhandler.cpp \
	gexceptionhandler.h \
	gexceptionsink.cpp \
	gexceptionsink.h \
	gexceptionsource.cpp \
	gexceptionsource.h \
	gfutureevent.h \
	ginterfaces.h \
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
	gnameservers.h \
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

# -- OS

if GCONFIG_WINDOWS

AM_CPPFLAGS = -I$(top_srcdir)/src/glib -I$(top_srcdir)/src/gssl -I$(top_srcdir)/src/win32

OS_SOURCES = \
	gdescriptor_win32.cpp \
	geventloop_win32.cpp \
	gfutureevent_win32.cpp \
	gnameservers_win32.cpp \
	gsocket_win32.cpp

OS_EXTRA_SOURCES =

OS_EXTRA_DIST = \
	gdescriptor_unix.cpp \
	gfutureevent_unix.cpp \
	gnameservers_unix.cpp \
	gsocket_unix.cpp

else

AM_CPPFLAGS = -I$(top_srcdir)/src/glib -I$(top_srcdir)/src/gssl -DG_LIB_SMALL

OS_SOURCES = \
	gdescriptor_unix.cpp \
	gfutureevent_unix.cpp \
	gnameservers_unix.cpp \
	gsocket_unix.cpp

OS_EXTRA_SOURCES =

OS_EXTRA_DIST = \
	gdescriptor_win32.cpp \
	geventloop_win32.cpp \
	gfutureevent_win32.cpp \
	gnameservers_win32.cpp \
	gsocket_win32.cpp

endif

# -- INTERFACE_NAMES

if GCONFIG_INTERFACE_NAMES

if GCONFIG_WINDOWS

INTERFACES_SOURCES = \
	ginterfaces_common.cpp \
	ginterfaces_win32.cpp

INTERFACES_EXTRA_SOURCES =

INTERFACES_EXTRA_DIST = \
	ginterfaces_unix.cpp \
	ginterfaces_none.cpp

else

INTERFACES_SOURCES = \
	ginterfaces_unix.cpp \
	ginterfaces_common.cpp

INTERFACES_EXTRA_SOURCES = \
	ginterfaces_none.cpp

INTERFACES_EXTRA_DIST = \
	ginterfaces_win32.cpp \
	ginterfaces_none.cpp

endif

else

INTERFACES_SOURCES = \
	ginterfaces_none.cpp

INTERFACES_EXTRA_SOURCES = \
	ginterfaces_unix.cpp \
	ginterfaces_common.cpp

INTERFACES_EXTRA_DIST = \
	ginterfaces_unix.cpp \
	ginterfaces_win32.cpp \
	ginterfaces_common.cpp

endif

# -- DNSBL

if GCONFIG_DNSBL

DNSBL_SOURCES = \
	gdnsbl.h \
	gdnsbl_enabled.cpp \
	gdnsblock.h \
	gdnsblock.cpp

DNSBL_EXTRA_SOURCES = \
	gdnsbl_disabled.cpp

DNSBL_EXTRA_DIST = \
	gdnsbl_disabled.cpp

else

DNSBL_SOURCES = \
	gdnsbl.h \
	gdnsbl_disabled.cpp

DNSBL_EXTRA_SOURCES = \
	gdnsbl_enabled.cpp \
	gdnsblock.cpp

DNSBL_EXTRA_DIST = \
	gdnsbl_enabled.cpp \
	gdnsblock.h \
	gdnsblock.cpp

endif

# -- UDS

if GCONFIG_UDS

if GCONFIG_WINDOWS

UDS_SOURCES = \
	gaddresslocal_none.cpp

UDS_EXTRA_SOURCES = \
	gaddresslocal_unix.cpp

else

UDS_SOURCES = \
	gaddresslocal_unix.cpp

UDS_EXTRA_SOURCES = \
	gaddresslocal_none.cpp

UDS_EXTRA_DIST = \
	gaddresslocal_none.cpp

endif

else

UDS_SOURCES = \
	gaddresslocal_none.cpp

UDS_EXTRA_SOURCES = \
	gaddresslocal_unix.cpp

UDS_EXTRA_DIST = \
	gaddresslocal_unix.cpp

endif

# -- EVENTLOOP

if GCONFIG_WINDOWS

EVENTLOOP_SOURCES = \
	geventloophandles.h \
	geventloophandles_st.cpp

EVENTLOOP_EXTRA_SOURCES =

EVENTLOOP_EXTRA_DIST = \
	geventloop_epoll.cpp \
	geventloop_select.cpp

else

if GCONFIG_EPOLL

EVENTLOOP_SOURCES = \
	geventloop_epoll.cpp

EVENTLOOP_EXTRA_SOURCES = \
	geventloop_select.cpp

EVENTLOOP_EXTRA_DIST = \
	geventloop_select.cpp \
	geventloophandles.h \
	geventloophandles_st.cpp

else

EVENTLOOP_SOURCES = \
	geventloop_select.cpp

EVENTLOOP_EXTRA_SOURCES = \
	geventloop_epoll.cpp

EVENTLOOP_EXTRA_DIST = \
	geventloop_epoll.cpp \
	geventloophandles.h \
	geventloophandles_st.cpp

endif

endif

# ===

EXTRA_DIST = \
	$(DNSBL_EXTRA_DIST) \
	$(EVENTLOOP_EXTRA_DIST) \
	$(INTERFACES_EXTRA_DIST) \
	$(OS_EXTRA_DIST) \
	$(UDS_EXTRA_DIST)

libgnet_a_SOURCES = \
	$(COMMON_SOURCES) \
	$(DNSBL_SOURCES) \
	$(EVENTLOOP_SOURCES) \
	$(INTERFACES_SOURCES) \
	$(OS_SOURCES) \
	$(UDS_SOURCES)

libgnetextra_a_SOURCES = \
	$(DNSBL_EXTRA_SOURCES) \
	$(EVENTLOOP_EXTRA_SOURCES) \
	$(INTERFACES_EXTRA_SOURCES) \
	$(OS_EXTRA_SOURCES) \
	$(UDS_EXTRA_SOURCES)

extra-local: libgnetextra.a

