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

noinst_LIBRARIES = libgverifiers.a

AM_CPPFLAGS = \
	-I$(top_srcdir)/src/glib \
	-I$(top_srcdir)/src/gssl \
	-I$(top_srcdir)/src/gnet \
	-I$(top_srcdir)/src/win32 \
	-I$(top_srcdir)/src/gstore \
	-I$(top_srcdir)/src/gsmtp \
	-DG_LIB_SMALL

libgverifiers_a_SOURCES = \
	gexecutableverifier.cpp \
	gexecutableverifier.h \
	gnetworkverifier.cpp \
	gnetworkverifier.h \
	ginternalverifier.cpp \
	ginternalverifier.h \
	guserverifier.cpp \
	guserverifier.h \
	gverifierfactory.cpp \
	gverifierfactory.h

