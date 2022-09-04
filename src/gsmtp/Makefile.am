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

if GCONFIG_WINDOWS
OS_SOURCES=gmessagestore_win32.cpp
OS_SOURCES_EXTRA_DIST=gmessagestore_unix.cpp
else
OS_SOURCES=gmessagestore_unix.cpp
OS_SOURCES_EXTRA_DIST=gmessagestore_win32.cpp
endif

ADMIN_SOURCES = gadminserver.cpp gadminserver.h
ADMIN_EXTRA_DIST =

EXTRA_DIST=\
	$(ADMIN_EXTRA_DIST) \
	$(OS_SOURCES_EXTRA_DIST)

AM_CPPFLAGS = \
	-I$(top_srcdir)/src/glib \
	-I$(top_srcdir)/src/gssl \
	-I$(top_srcdir)/src/gnet \
	-I$(top_srcdir)/src/gauth \
	-D "G_SPOOLDIR=$(e_spooldir)"

noinst_LIBRARIES = libgsmtp.a

libgsmtp_a_SOURCES = \
	$(ADMIN_SOURCES) \
	$(OS_SOURCES) \
	genvelope.cpp \
	genvelope.h \
	gexecutablefilter.cpp \
	gexecutablefilter.h \
	gexecutableverifier.cpp \
	gexecutableverifier.h \
	gfactoryparser.cpp \
	gfactoryparser.h \
	gfilestore.cpp \
	gfilestore.h \
	gfilter.cpp \
	gfilter.h \
	gfilterchain.cpp \
	gfilterchain.h \
	gfilterfactory.cpp \
	gfilterfactory.h \
	ginternalverifier.cpp \
	ginternalverifier.h \
	gmessagestore.cpp \
	gmessagestore.h \
	gnetworkfilter.cpp \
	gnetworkfilter.h \
	gnetworkverifier.cpp \
	gnetworkverifier.h \
	gnewfile.cpp \
	gnewfile.h \
	gnewmessage.cpp \
	gnewmessage.h \
	gnullfilter.cpp \
	gnullfilter.h \
	gprotocolmessage.cpp \
	gprotocolmessage.h \
	gprotocolmessageforward.cpp \
	gprotocolmessageforward.h \
	gprotocolmessagestore.cpp \
	gprotocolmessagestore.h \
	grequestclient.cpp \
	grequestclient.h \
	gsmtpclient.cpp \
	gsmtpclient.h \
	gsmtpclientprotocol.cpp \
	gsmtpclientprotocol.h \
	gsmtpserver.cpp \
	gsmtpserver.h \
	gsmtpserverprotocol.cpp \
	gsmtpserverprotocol.h \
	gspamclient.cpp \
	gspamclient.h \
	gspamfilter.cpp \
	gspamfilter.h \
	gstoredfile.cpp \
	gstoredfile.h \
	gstoredmessage.cpp \
	gstoredmessage.h \
	gverifier.cpp \
	gverifier.h \
	gverifierfactory.cpp \
	gverifierfactory.h \
	gverifierstatus.cpp \
	gverifierstatus.h

