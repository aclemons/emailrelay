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

if POP
POP_EXTRA_DIST= gpop_disabled.cpp
POP_SOURCES= \
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
else
POP_EXTRA_DIST= \
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
POP_SOURCES= gpop_disabled.cpp
endif

EXTRA_DIST=\
	$(POP_EXTRA_DIST) \
	mingw.mak \
	gpopsecrets_win32.cpp

INCLUDES = -I$(top_srcdir)/lib/$(COMPILER_VERSION) -I$(top_srcdir)/src/glib -I$(top_srcdir)/src/gssl -I$(top_srcdir)/src/gnet -I$(top_srcdir)/src/gauth -DG_SPOOLDIR=\"$(e_spooldir)\" -DG_SYSCONFDIR=\"$(e_sysconfdir)\"

noinst_LIBRARIES = libgpop.a

libgpop_a_SOURCES = $(POP_SOURCES)

