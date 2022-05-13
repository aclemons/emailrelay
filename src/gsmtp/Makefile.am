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


EXTRA_DIST=\
	$(OS_SOURCES_EXTRA_DIST)

AM_CPPFLAGS = \
	-I$(top_srcdir)/src/glib \
	-I$(top_srcdir)/src/gssl \
	-I$(top_srcdir)/src/gnet \
	-I$(top_srcdir)/src/gauth \
	-D "G_SPOOLDIR=$(e_spooldir)"

noinst_LIBRARIES = libgsmtp.a

libgsmtp_a_SOURCES = \
	$(OS_SOURCES) \
	gadminserver.cpp \
	gadminserver.h \
	gexecutablefilter.cpp \
	gexecutablefilter.h \
	gexecutableverifier.cpp \
	gexecutableverifier.h \
	gnetworkfilter.cpp \
	gnetworkfilter.h \
	gnetworkverifier.cpp \
	gnetworkverifier.h \
	gfilterchain.cpp \
	gfilterchain.h \
	gfilterfactory.cpp \
	grequestclient.cpp \
	grequestclient.h \
	gspamclient.cpp \
	gspamclient.h \
	gspamfilter.cpp \
	gspamfilter.h \
	gverifierfactory.cpp \
	genvelope.cpp \
	genvelope.h \
	gfactoryparser.cpp \
	gfactoryparser.h \
	gfilestore.cpp \
	gfilestore.h \
	gfilter.cpp \
	gfilter.h \
	gfilterfactory.h \
	ginternalverifier.cpp \
	ginternalverifier.h \
	gmessagestore.cpp \
	gmessagestore.h \
	gnullfilter.cpp \
	gnullfilter.h \
	gnewfile.cpp \
	gnewfile.h \
	gnewmessage.cpp \
	gnewmessage.h \
	gprotocolmessage.cpp \
	gprotocolmessageforward.cpp \
	gprotocolmessageforward.h \
	gprotocolmessage.h \
	gprotocolmessagestore.cpp \
	gprotocolmessagestore.h \
	gsmtpclient.cpp \
	gsmtpclient.h \
	gsmtpclientprotocol.cpp \
	gsmtpclientprotocol.h \
	gsmtpclientreply.cpp \
	gsmtpclientreply.h \
	gsmtpserver.cpp \
	gsmtpserver.h \
	gsmtpserverbuffer.cpp \
	gsmtpserverbuffer.h \
	gsmtpserverparser.cpp \
	gsmtpserverparser.h \
	gsmtpserverprotocol.cpp \
	gsmtpserverprotocol.h \
	gsmtpserversend.cpp \
	gsmtpserversend.h \
	gsmtpserversender.h \
	gsmtpservertext.cpp \
	gsmtpservertext.h \
	gstoredfile.cpp \
	gstoredfile.h \
	gstoredmessage.cpp \
	gstoredmessage.h \
	gverifier.cpp \
	gverifier.h \
	gverifierfactory.h \
	gverifierstatus.cpp \
	gverifierstatus.h

