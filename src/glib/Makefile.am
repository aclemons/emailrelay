#
## Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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

AM_CPPFLAGS = -DG_LIB_SMALL

noinst_LIBRARIES = libglib.a

UNIX_SOURCES = \
	gcleanup_unix.cpp \
	gconvert_unix.cpp \
	gdaemon_unix.cpp \
	gdirectory_unix.cpp \
	genvironment_unix.cpp \
	gexecutablecommand_unix.cpp \
	gfile_unix.cpp \
	ghostname_unix.cpp \
	gidentity_unix.cpp \
	glogoutput_unix.cpp \
	gmsg_unix.cpp \
	gnewprocess_unix.cpp \
	gprocess_unix.cpp

WINDOWS_SOURCES = \
	gcleanup_win32.cpp \
	gconvert_win32.cpp \
	gdaemon_win32.cpp \
	gdirectory_win32.cpp \
	genvironment_win32.cpp \
	gexecutablecommand_win32.cpp \
	gfile_win32.cpp \
	ghostname_win32.cpp \
	gidentity_win32.cpp \
	glogoutput_win32.cpp \
	gmsg_win32.cpp \
	gnewprocess_win32.cpp \
	gprocess_win32.cpp

COMMON_SOURCES = \
	galign.h \
	garg.h \
	garg.cpp \
	gassert.h \
	gbase64.h \
	gbase64.cpp \
	gbatchfile.h \
	gbatchfile.cpp \
	gbuffer.h \
	gcall.h \
	gcleanup.h \
	gconvert.h \
	gconvert.cpp \
	gdaemon.h \
	gdate.h \
	gdate.cpp \
	gdatetime.h \
	gdatetime.cpp \
	gdef.h \
	gdirectory.h \
	gdirectory.cpp \
	geightbit.h \
	genvironment.h \
	genvironment.cpp \
	gexception.h \
	gexception.cpp \
	gexecutablecommand.h \
	gexecutablecommand.cpp \
	gfbuf.h \
	gfile.h \
	gfile.cpp \
	gformat.h \
	gformat.cpp \
	ggetopt.h \
	ggetopt.cpp \
	ghash.h \
	ghash.cpp \
	ghashstate.h \
	ghostname.h \
	gidentity.h \
	gimembuf.h \
	glimits.h \
	glog.h \
	glog.cpp \
	glogstream.h \
	glogstream.cpp \
	glogoutput.h \
	glogoutput.cpp \
	gstrmacros.h \
	gmapfile.h \
	gmapfile.cpp \
	gmd5.h \
	gmd5.cpp \
	gmsg.h \
	gnewprocess.h \
	gomembuf.h \
	goptional.h \
	goption.h \
	goption.cpp \
	goptionmap.h \
	goptionmap.cpp \
	goptionparser.h \
	goptionparser.cpp \
	goptions.h \
	goptions.cpp \
	goptionsusage.h \
	goptionsusage.cpp \
	goptionvalue.h \
	gpath.h \
	gpath.cpp \
	gpidfile.h \
	gpidfile.cpp \
	gprocess.h \
	grandom.h \
	grandom.cpp \
	greadwrite.h \
	greadwrite.cpp \
	groot.h \
	groot.cpp \
	gscope.h \
	gsignalsafe.h \
	gsleep.h \
	gslot.h \
	gslot.cpp \
	gstatemachine.h \
	gstr.h \
	gstr.cpp \
	gstringarray.h \
	gstringfield.h \
	gstringtoken.h \
	gstringlist.h \
	gstringlist.cpp \
	gstringmap.h \
	gstrings.h \
	gstringwrap.cpp \
	gstringwrap.h \
	gstringview.h \
	gstringview.cpp \
	gtest.h \
	gtest.cpp \
	gthread.cpp \
	gtime.h \
	gtime.cpp \
	gxtext.h \
	gxtext.cpp

# -- PAM

if GCONFIG_PAM
if GCONFIG_WINDOWS
PAM_SOURCES = \
	gpam.h \
	gpam_none.cpp
PAM_EXTRA_DIST = \
	gpam_linux.cpp
else
PAM_SOURCES = \
	gpam.h \
	gpam_linux.cpp
PAM_EXTRA_DIST = \
	gpam_none.cpp
endif
else
PAM_SOURCES = \
	gpam.h \
	gpam_none.cpp
PAM_EXTRA_DIST = \
	gpam_linux.cpp
endif

# -- GETTEXT

if GCONFIG_GETTEXT
if GCONFIG_WINDOWS
GETTEXT_SOURCES = \
	ggettext.h \
	ggettext_win32.cpp
GETTEXT_EXTRA_DIST = \
	ggettext_none.cpp \
	ggettext_unix.cpp
else
GETTEXT_SOURCES = \
	ggettext.h \
	ggettext_unix.cpp
GETTEXT_EXTRA_DIST = \
	ggettext_none.cpp \
	ggettext_win32.cpp
endif
else
GETTEXT_SOURCES = \
	ggettext_none.cpp \
	ggettext.h
GETTEXT_EXTRA_DIST = \
	ggettext_win32.cpp \
	ggettext_unix.cpp
endif

# -- OS

if GCONFIG_WINDOWS
OS_SOURCES = \
	$(COMMON_SOURCES) \
	$(WINDOWS_SOURCES)
OS_EXTRA_DIST = \
	$(UNIX_SOURCES)
else
OS_SOURCES = \
	$(COMMON_SOURCES) \
	$(UNIX_SOURCES)
OS_EXTRA_DIST = \
	$(WINDOWS_SOURCES)
endif

# ===

libglib_a_SOURCES = \
	$(GETTEXT_SOURCES) \
	$(OS_SOURCES) \
	$(PAM_SOURCES)

EXTRA_DIST = \
	$(GETTEXT_EXTRA_DIST) \
	$(OS_EXTRA_DIST) \
	$(PAM_EXTRA_DIST)

