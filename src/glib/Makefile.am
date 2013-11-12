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

INCLUDES = -I$(top_srcdir)/lib/$(COMPILER_VERSION) $(PAM_INCLUDE)

noinst_LIBRARIES = libglib.a

if PAM
PAM_EXTRA_DIST = gpam_none.cpp
PAM_SOURCES = gpam_linux.cpp
else
PAM_EXTRA_DIST = gpam_linux.cpp
PAM_SOURCES = gpam_none.cpp
endif

if MAC
MAC_EXTRA_DIST = garg_unix.cpp
MAC_SOURCES = garg_mac.cpp
else
MAC_EXTRA_DIST = garg_mac.cpp
MAC_SOURCES = garg_unix.cpp
endif

if EXEC
EXEC_EXTRA_DIST = gnewprocess_unix_exec_disabled.cpp
EXEC_SOURCES = gnewprocess_unix_exec_enabled.cpp
else
EXEC_EXTRA_DIST = gnewprocess_unix_exec_enabled.cpp
EXEC_SOURCES = gnewprocess_unix_exec_disabled.cpp
endif

if IDENTITY
IDENTITY_EXTRA_DIST = gidentity_disabled.cpp
IDENTITY_SOURCES = gidentity_unix.cpp
else
IDENTITY_EXTRA_DIST = gidentity_unix.cpp
IDENTITY_SOURCES = gidentity_disabled.cpp
endif

if GLOB
GLOB_EXTRA_DIST = gdirectory_unix_noglob.cpp
GLOB_SOURCES = gdirectory_unix_glob.cpp
else
GLOB_EXTRA_DIST = gdirectory_unix_glob.cpp
GLOB_SOURCES = gdirectory_unix_noglob.cpp
endif


EXTRA_DIST=\
	$(EXEC_EXTRA_DIST) \
	$(FRAGMENTS_EXTRA_DIST) \
	$(IDENTITY_EXTRA_DIST) \
	$(GLOB_EXTRA_DIST) \
	$(MAC_EXTRA_DIST) \
	$(PAM_EXTRA_DIST) \
	garg_win32.cpp \
	gcleanup_win32.cpp \
	gconvert_win32.cpp \
	gdaemon_win32.cpp \
	gdatetime_win32.cpp \
	gdirectory_win32.cpp \
	genvironment_win32.cpp \
	gexecutable_win32.cpp \
	gfile_win32.cpp \
	gfs_win32.cpp \
	ghostname_win32.cpp \
	gidentity_win32.cpp \
	glogoutput_win32.cpp \
	gprocess_win32.cpp \
	gnewprocess_win32.cpp \
	mingw.mak

libglib_a_SOURCES = \
	$(EXEC_SOURCES) \
	$(FRAGMENTS_SOURCES) \
	$(IDENTITY_SOURCES) \
	$(GLOB_SOURCES) \
	$(MAC_SOURCES) \
	$(PAM_SOURCES) \
	md5.cpp \
	md5.h \
	garg.cpp \
	garg.h \
	gassert.h \
	gbase64.cpp \
	gbase64.h \
	gcleanup.h \
	gcleanup_unix.cpp \
	gconvert.h \
	gconvert.cpp \
	gconvert_unix.cpp \
	gcounter.cpp \
	gcounter.h \
	gdaemon.h \
	gdaemon_unix.cpp \
	gdate.h \
	gdate.cpp \
	gdatetime.cpp \
	gdatetime.h \
	gdatetime_unix.cpp \
	gdebug.h \
	gdef.h \
	gdirectory.cpp \
	gdirectory.h \
	gdirectory_unix.cpp \
	genvironment.h \
	genvironment_unix.cpp \
	gexception.h \
	gexception.cpp \
	gexecutable_unix.cpp \
	gexecutable.cpp \
	gexecutable.h \
	gfile.h \
	gfile.cpp \
	gfile_unix.cpp \
	gfs.h \
	gfs_unix.cpp \
	ggetopt.h \
	ggetopt.cpp \
	ghostname_unix.cpp \
	ghostname.h \
	gidentity.h \
	glimits.h \
	glog.cpp \
	glog.h \
	glogoutput.h \
	glogoutput.cpp \
	glogoutput_unix.cpp \
	gnewprocess.h \
	gmd5_native.cpp \
	gmd5.h \
	gmemory.h \
	gnoncopyable.h \
	gpam.h \
	gpam.cpp \
	gpath.h \
	gpath.cpp \
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
	gstr.cpp \
	gstr.h \
	gstrings.cpp \
	gstrings.h \
	gtest.cpp \
	gtest.h \
	gtime.cpp \
	gtime.h \
	gxtext.cpp \
	gxtext.h

