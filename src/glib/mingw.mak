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
	garg.cpp \
	garg_win32.cpp \
	gbase64.cpp \
	gcleanup_win32.cpp \
	gconvert.cpp \
	gconvert_win32.cpp \
	gcounter.cpp \
	gdaemon_win32.cpp \
	gdate.cpp \
	gdatetime.cpp \
	gdatetime_unix.cpp \
	gdirectory.cpp \
	gdirectory_win32.cpp \
	genvironment_unix.cpp \
	gexception.cpp \
	gexecutable.cpp \
	gexecutable_win32.cpp \
	gfile.cpp \
	gfile_win32.cpp \
	gfs_win32.cpp \
	ggetopt.cpp \
	ghostname_win32.cpp \
	gidentity_win32.cpp \
	glog.cpp \
	glogoutput.cpp \
	glogoutput_win32.cpp \
	gmd5_native.cpp \
	gpath.cpp \
	gpam.cpp \
	gpam_none.cpp \
	gpidfile.cpp \
	gprocess_win32.cpp \
	gnewprocess_win32.cpp \
	groot.cpp \
	gslot.cpp \
	gstr.cpp \
	gstrings.cpp \
	gtest.cpp \
	gtime.cpp \
	gxtext.cpp \
	md5.cpp

mk_target=glib.a

all: $(mk_target)

include ../mingw-common.mak

$(mk_target): $(mk_objects)
	$(mk_ar) $(mk_target) $(mk_objects)

