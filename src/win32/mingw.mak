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
#
# mingw.mak
#

mk_sources=\
	gappbase.cpp \
	gappinst.cpp \
	gcontrol.cpp \
	gcracker.cpp \
	gdc.cpp \
	gdialog.cpp \
	gpump.cpp \
	gpump_dialog.cpp \
	gscmap.cpp \
	gtray.cpp \
	gwinbase.cpp \
	gwindow.cpp \
	gwinhid.cpp

mk_target=gwin32.a

all: $(mk_target)

include ../mingw-common.mak

$(mk_target): $(mk_objects)
	$(mk_ar) $(mk_target) $(mk_objects)

