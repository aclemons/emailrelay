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

ADMIN_SOURCES_ = gadminserver.cpp gadminserver.h

if ADMIN
ADMIN_EXTRA_DIST =
ADMIN_SOURCES = $(ADMIN_SOURCES_)
else
ADMIN_EXTRA_DIST = $(ADMIN_SOURCES_)
ADMIN_SOURCES =
endif

EXEC_SOURCES_=\
	gexecutableprocessor.cpp \
	gexecutableprocessor.h \
	gexecutableverifier.cpp \
	gexecutableverifier.h \
	gnetworkprocessor.cpp \
	gnetworkprocessor.h \
	gnetworkverifier.cpp \
	gnetworkverifier.h \
	gprocessorfactory_exec_enabled.cpp \
	grequestclient.cpp \
	grequestclient.h \
	gspamclient.cpp \
	gspamclient.h \
	gspamprocessor.cpp \
	gspamprocessor.h \
	gverifierfactory_exec_enabled.cpp

if EXEC
EXEC_EXTRA_DIST = gprocessorfactory_exec_disabled.cpp gverifierfactory_exec_disabled.cpp
EXEC_SOURCES = $(EXEC_SOURCES_)
else
EXEC_EXTRA_DIST = $(EXEC_SOURCES_)
EXEC_SOURCES = gprocessorfactory_exec_disabled.cpp gverifierfactory_exec_disabled.cpp
endif

EXTRA_DIST=\
	$(ADMIN_EXTRA_DIST) \
	$(EXEC_EXTRA_DIST) \
	gmessagestore_win32.cpp \
	mingw.mak

INCLUDES = -I$(top_srcdir)/lib/$(COMPILER_VERSION) -I$(top_srcdir)/src/glib -I$(top_srcdir)/src/gssl -I$(top_srcdir)/src/gnet -I$(top_srcdir)/src/gauth -DG_SPOOLDIR=\"$(e_spooldir)\"

noinst_LIBRARIES = libgsmtp.a

libgsmtp_a_SOURCES = \
	$(ADMIN_SOURCES) \
	$(EXEC_SOURCES) \
	gclientprotocol.cpp \
	gclientprotocol.h \
	gfactoryparser.cpp \
	gfactoryparser.h \
	gfilestore.cpp \
	gfilestore.h \
	ginternalverifier.cpp \
	ginternalverifier.h \
	gmessagestore.cpp \
	gmessagestore.h \
	gmessagestore_unix.cpp \
	gnullprocessor.cpp \
	gnullprocessor.h \
	gprocessorfactory.h \
	gnewfile.cpp \
	gnewfile.h \
	gnewmessage.cpp \
	gnewmessage.h \
	gprocessor.cpp \
	gprocessor.h \
	gprotocolmessage.cpp \
	gprotocolmessageforward.cpp \
	gprotocolmessageforward.h \
	gprotocolmessage.h \
	gprotocolmessagestore.cpp \
	gprotocolmessagestore.h \
	gserverprotocol.cpp \
	gserverprotocol.h \
	gsmtpclient.cpp \
	gsmtpclient.h \
	gsmtp.h \
	gsmtpserver.cpp \
	gsmtpserver.h \
	gstoredfile.cpp \
	gstoredfile.h \
	gstoredmessage.cpp \
	gstoredmessage.h \
	gverifier.cpp \
	gverifier.h \
	gverifierfactory.h \
	gverifierstatus.cpp \
	gverifierstatus.h

