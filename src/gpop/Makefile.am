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

noinst_LIBRARIES = libgpop.a

EXTRA_LIBRARIES = libgpopextra.a

AM_CPPFLAGS = \
	-I$(top_srcdir)/src/glib \
	-I$(top_srcdir)/src/gssl \
	-I$(top_srcdir)/src/gnet \
	-I$(top_srcdir)/src/gauth \
	-D "G_SPOOLDIR=$(e_spooldir)" \
	-D "G_SYSCONFDIR=$(e_sysconfdir)" \
	-D G_LIB_SMALL

if GCONFIG_POP

POP_SOURCES = \
	gpop_enabled.cpp \
	gpop.h \
	gpopserver.cpp \
	gpopserver.h \
	gpopstore.cpp \
	gpopstore.h \
	gpopserverprotocol.cpp \
	gpopserverprotocol.h

POP_EXTRA_SOURCES = \
	gpop_disabled.cpp

POP_EXTRA_DIST = \
	gpop_disabled.cpp

else

POP_SOURCES = \
	gpop_disabled.cpp \
	gpop.h

POP_EXTRA_SOURCES = \
	gpop_enabled.cpp \
	gpopserver.cpp \
	gpopstore.cpp \
	gpopserverprotocol.cpp

POP_EXTRA_DIST = \
	gpop_enabled.cpp \
	gpopserver.cpp \
	gpopserver.h \
	gpopstore.cpp \
	gpopstore.h \
	gpopserverprotocol.cpp \
	gpopserverprotocol.h

endif

EXTRA_DIST = \
	$(POP_EXTRA_DIST)

libgpop_a_SOURCES = \
	$(POP_SOURCES)

libgpopextra_a_SOURCES = \
	$(POP_EXTRA_SOURCES)

extra-local: libgpopextra.a

