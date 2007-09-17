#
## Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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

EXTRA_DIST=\
	gmessagestore_win32.cpp \
	gsasl_gnu.cpp \
	mingw.mak

INCLUDES = -I$(top_srcdir)/lib/$(COMPILER_VERSION) -I$(top_srcdir)/src/glib -I$(top_srcdir)/src/gssl -I$(top_srcdir)/src/gnet -DG_SPOOLDIR=\"$(e_spooldir)\"

noinst_LIBRARIES = libgsmtp.a

libgsmtp_a_SOURCES = \
	gadminserver.cpp \
	gadminserver.h \
	gbase64.cpp \
	gbase64.h \
	gclientprotocol.cpp \
	gclientprotocol.h \
	gexecutableprocessor.cpp \
	gexecutableprocessor.h \
	gexecutableverifier.cpp \
	gexecutableverifier.h \
	gfactoryparser.cpp \
	gfactoryparser.h \
	gfilestore.cpp \
	gfilestore.h \
	gmessagestore.cpp \
	gmessagestore.h \
	gmessagestore_unix.cpp \
	gnetworkprocessor.cpp \
	gnetworkprocessor.h \
	gnetworkverifier.cpp \
	gnetworkverifier.h \
	gnewfile.cpp \
	gnewfile.h \
	gnewmessage.cpp \
	gnewmessage.h \
	gnullprocessor.cpp \
	gnullprocessor.h \
	gprocessor.cpp \
	gprocessor.h \
	gprocessorfactory.cpp \
	gprocessorfactory.h \
	gprotocolmessage.cpp \
	gprotocolmessageforward.cpp \
	gprotocolmessageforward.h \
	gprotocolmessage.h \
	gprotocolmessagestore.cpp \
	gprotocolmessagestore.h \
	grequestclient.cpp \
	grequestclient.h \
	gsasl.h \
	gsasl_native.cpp \
	gsecrets.cpp \
	gsecrets.h \
	gserverprotocol.cpp \
	gserverprotocol.h \
	gsmtpclient.cpp \
	gsmtpclient.h \
	gsmtp.h \
	gsmtpserver.cpp \
	gsmtpserver.h \
	gspamclient.cpp \
	gspamclient.h \
	gspamprocessor.cpp \
	gspamprocessor.h \
	gstoredfile.cpp \
	gstoredfile.h \
	gstoredmessage.cpp \
	gstoredmessage.h \
	gverifier.cpp \
	gverifier.h \
	gverifierfactory.cpp \
	gverifierfactory.h

