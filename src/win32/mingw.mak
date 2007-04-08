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

