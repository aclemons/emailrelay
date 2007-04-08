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

EXTRA_DIST=\
	mingw.mak \
	gpopsecrets_win32.cpp

INCLUDES = -I$(top_srcdir)/lib/$(COMPILER_VERSION) -I$(top_srcdir)/src/glib -I$(top_srcdir)/src/gnet -I$(top_srcdir)/src/gsmtp -DG_SPOOLDIR=\"$(e_spooldir)\" -DG_SYSCONFDIR=\"$(sysconfdir)\"

noinst_LIBRARIES = libgpop.a libgnopop.a

libgpop_a_SOURCES = \
	gpopauth.cpp \
	gpopauth.h \
	gpop.h \
	gpopsecrets.cpp \
	gpopsecrets_unix.cpp \
	gpopsecrets.h \
	gpopserver.cpp \
	gpopserver.h \
	gpopstore.cpp \
	gpopstore.h \
	gpopserverprotocol.cpp \
	gpopserverprotocol.h

libgnopop_a_SOURCES = gnopop.cpp


