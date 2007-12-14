#
## Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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

if EXEC
EXEC_EXTRA_DIST = gnewprocess_unix_exec_disabled.cpp
EXEC_SOURCES = gnewprocess_unix_exec_enabled.cpp
else
EXEC_EXTRA_DIST = gnewprocess_unix_exec_enabled.cpp
EXEC_SOURCES = gnewprocess_unix_exec_disabled.cpp
endif

if FRAGMENTS
FRAGMENTS_EXTRA_DIST = gstr.cpp
FRAGMENTS_SOURCES =
FRAGMENTS_NODIST_SOURCES = \
L_str_replace.cpp \
L_str_replaceAll_1.cpp \
L_str_replaceAll_2.cpp \
L_str_removeAll.cpp \
L_str_trimLeft.cpp \
L_str_trimRight.cpp \
L_str_trim.cpp \
L_str_trimmed.cpp \
L_str_isNumeric.cpp \
L_str_isPrintableAscii.cpp \
L_str_isUShort.cpp \
L_str_isUInt.cpp \
L_str_isULong.cpp \
L_str_fromBool.cpp \
L_str_fromDouble.cpp \
L_str_fromInt.cpp \
L_str_fromLong.cpp \
L_str_fromShort.cpp \
L_str_fromUInt.cpp \
L_str_fromULong.cpp \
L_str_fromUShort.cpp \
L_str_toBool.cpp \
L_str_toDouble.cpp \
L_str_toInt.cpp \
L_str_toLong.cpp \
L_str_toShort.cpp \
L_str_toUInt.cpp \
L_str_toULong.cpp \
L_str_toUShort.cpp \
L_str_lower.cpp \
L_str_upper.cpp \
L_str_printable.cpp \
L_str_readLineFrom.cpp \
L_str_wrap.cpp \
L_str_splitIntoTokens.cpp \
L_str_splitIntoFields.cpp \
L_str_join.cpp \
L_str_keys.cpp \
L_str_ws.cpp \
L_str_head.cpp \
L_str_tail.cpp \
L_str_tailMatch.cpp \
L_str_replace.cpp
else
FRAGMENTS_NODIST_SOURCES =
FRAGMENTS_EXTRA_DIST =
FRAGMENTS_SOURCES = gstr.cpp
endif

EXTRA_DIST=\
	$(EXEC_EXTRA_DIST) \
	$(FRAGMENTS_EXTRA_DIST) \
	garg_win32.cpp \
	gcleanup_win32.cpp \
	gdaemon_win32.cpp \
	gdatetime_win32.cpp \
	gdirectory_win32.cpp \
	gexecutable_win32.cpp \
	gfile_win32.cpp \
	gfs_win32.cpp \
	ghostname_win32.cpp \
	gidentity_win32.cpp \
	glogoutput_win32.cpp \
	gprocess_win32.cpp \
	gnewprocess_win32.cpp \
	mingw.mak

INCLUDES = -I$(top_srcdir)/lib/$(COMPILER_VERSION)

noinst_LIBRARIES = libglib.a

nodist_libglib_a_SOURCES = $(FRAGMENTS_NODIST_SOURCES)

libglib_a_SOURCES = \
	$(EXEC_SOURCES) \
	$(FRAGMENTS_SOURCES) \
	md5.cpp \
	md5.h \
	garg.cpp \
	garg.h \
	garg_unix.cpp \
	gassert.h \
	gconvert.h \
	gcleanup.h \
	gcleanup_unix.cpp \
	gcounter.cpp \
	gcounter.h \
	gdaemon.h \
	gdaemon_unix.cpp \
	gdate.cpp \
	gdate.h \
	gdatetime.cpp \
	gdatetime.h \
	gdatetime_unix.cpp \
	gdebug.h \
	gdef.h \
	gdirectory.cpp \
	gdirectory.h \
	gdirectory_unix.cpp \
	gexception.cpp \
	gexception.h \
	gexecutable_unix.cpp \
	gexecutable.cpp \
	gexecutable.h \
	gfile.cpp \
	gfile.h \
	gfile_unix.cpp \
	gfs.h \
	gfs_unix.cpp \
	ggetopt.h \
	ggetopt.cpp \
	ghostname_unix.cpp \
	ghostname.h \
	gidentity.h \
	gidentity_unix.cpp \
	glimits.h \
	glog.cpp \
	glog.h \
	glogoutput.cpp \
	glogoutput.h \
	glogoutput_unix.cpp \
	gnewprocess.h \
	gmd5_native.cpp \
	gmd5.h \
	gmemory.h \
	gnoncopyable.h \
	gpath.cpp \
	gpath.h \
	gpidfile.cpp \
	gpidfile.h \
	gprocess.h \
	gprocess_unix.cpp \
	groot.cpp \
	groot.h \
	gsetter.h \
	gsignalsafe.h \
	gsleep.h \
	gslot.cpp \
	gslot.h \
	gstatemachine.h \
	gstr.h \
	gstrings.h \
	gtest.cpp \
	gtest.h \
	gtime.cpp \
	gtime.h \
	gxtext.cpp \
	gxtext.h

