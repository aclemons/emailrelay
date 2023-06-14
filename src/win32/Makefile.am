#
## Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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

AM_CPPFLAGS = -I$(top_srcdir)/src/glib

noinst_LIBRARIES = libwin32.a

if GCONFIG_WINDOWS

EXTRA_DIST = empty.cpp

libwin32_a_SOURCES = \
	gappinst.cpp \
	gappinst.h \
	gappbase.cpp \
	gappbase.h \
	gcracker.cpp \
	gcracker.h \
	gdc.h \
	gdc.cpp \
	gdialog.cpp \
	gdialog.h \
	gcontrol.cpp \
	gcontrol.h \
	gpump.cpp \
	gpump.h \
	gscmap.h \
	gscmap.cpp \
	gsize.h \
	gstack.cpp \
	gstack.h \
	gtray.h \
	gtray.cpp \
	gwinbase.cpp \
	gwinbase.h \
	gwindow.cpp \
	gwindow.h

else

EXTRA_DIST = \
	gappinst.cpp \
	gappinst.h \
	gappbase.cpp \
	gappbase.h \
	gcracker.cpp \
	gcracker.h \
	gdc.h \
	gdc.cpp \
	gdialog.cpp \
	gdialog.h \
	gcontrol.cpp \
	gcontrol.h \
	gpump.cpp \
	gpump.h \
	gscmap.h \
	gscmap.cpp \
	gsize.h \
	gtray.h \
	gtray.cpp \
	gwinbase.cpp \
	gwinbase.h \
	gwindow.cpp \
	gwindow.h

libwin32_a_SOURCES = empty.cpp

endif
