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
	md5.cpp \
	garg.cpp \
	garg_win32.cpp \
	gcleanup_win32.cpp \
	gexe.cpp \
	gdaemon_win32.cpp \
	gdate.cpp \
	gdatetime.cpp \
	gdatetime_win32.cpp \
	gdirectory.cpp \
	gdirectory_win32.cpp \
	gexception.cpp \
	gfile.cpp \
	gfile_win32.cpp \
	gfs_win32.cpp \
	ggetopt.cpp \
	gidentity_win32.cpp \
	glog.cpp \
	glogoutput.cpp \
	glogoutput_win32.cpp \
	gmd5_native.cpp \
	gpath.cpp \
	gpidfile.cpp \
	gprocess_win32.cpp \
	gregistry_win32.cpp \
	groot.cpp \
	gslot.cpp \
	gstr.cpp \
	gtime.cpp \
	gxtext.cpp

mk_target=glib.a

all: $(mk_target)

include ../mingw-common.mak

$(mk_target): $(mk_objects)
	$(mk_ar) $(mk_target) $(mk_objects)

