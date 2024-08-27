#
## Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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

AM_CPPFLAGS = \
	-I$(top_srcdir)/src/glib \
	-I$(top_srcdir)/src/gssl \
	-I$(top_srcdir)/src/gnet \
	-I$(top_srcdir)/src/gauth \
	-I$(top_srcdir)/src/gstore \
	-D "G_SPOOLDIR=$(e_spooldir)" \
	-D G_LIB_SMALL

noinst_LIBRARIES = libgsmtp.a

EXTRA_LIBRARIES = libgsmtpextra.a

if GCONFIG_ADMIN
 ADMIN_SOURCES = gadminserver.h gadminserver_enabled.cpp
 ADMIN_EXTRA_DIST = gadminserver_disabled.cpp
 ADMIN_EXTRA_SOURCES = gadminserver.h gadminserver_disabled.cpp
else
 ADMIN_SOURCES = gadminserver.h gadminserver_disabled.cpp
 ADMIN_EXTRA_DIST = gadminserver_enabled.cpp
 ADMIN_EXTRA_SOURCES = gadminserver.h gadminserver_enabled.cpp
endif

EXTRA_DIST = \
	$(ADMIN_EXTRA_DIST)

libgsmtp_a_SOURCES = \
	$(ADMIN_SOURCES) \
	grequestclient.cpp \
	grequestclient.h \
	gspamclient.cpp \
	gspamclient.h \
	gfilter.cpp \
	gfilter.h \
	gfilterfactorybase.cpp \
	gfilterfactorybase.h \
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
	gsmtpforward.cpp \
	gsmtpforward.h \
	gsmtpserver.cpp \
	gsmtpserver.h \
	gsmtpserverbufferin.cpp \
	gsmtpserverbufferin.h \
	gsmtpserverflowcontrol.h \
	gsmtpserverparser.cpp \
	gsmtpserverparser.h \
	gsmtpserverprotocol.cpp \
	gsmtpserverprotocol.h \
	gsmtpserversend.cpp \
	gsmtpserversend.h \
	gsmtpserversender.h \
	gsmtpservertext.cpp \
	gsmtpservertext.h \
	gverifier.cpp \
	gverifier.h \
	gverifierfactorybase.cpp \
	gverifierfactorybase.h \
	gverifierstatus.cpp \
	gverifierstatus.h

libgsmtpextra_a_SOURCES = \
	$(ADMIN_EXTRA_SOURCES)

extra-local: libgsmtpextra.a

