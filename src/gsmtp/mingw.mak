#
## Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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
	gadminserver.cpp \
	gbase64.cpp \
	gclientprotocol.cpp \
	gfilestore.cpp \
	gmessagestore.cpp \
	gmessagestore_win32.cpp \
	gnewfile.cpp \
	gnewmessage.cpp \
	gprocessor.cpp \
	gprotocolmessage.cpp \
	gprotocolmessageforward.cpp \
	gprotocolmessagestore.cpp \
	gprotocolmessagescanner.cpp \
	gsasl_native.cpp \
	gsecrets.cpp \
	gserverprotocol.cpp \
	gscannerclient.cpp \
	gsmtpclient.cpp \
	gsmtpserver.cpp \
	gstoredfile.cpp \
	gstoredmessage.cpp \
	gverifier.cpp

mk_target=gsmtp.a

all: $(mk_target)

include ../mingw-common.mak

$(mk_target): $(mk_objects)
	$(mk_ar) $(mk_target) $(mk_objects)

