#
## Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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

AM_CPPFLAGS =

noinst_LIBRARIES = libglib.a

if GCONFIG_WINDOWS

EXTRA_DIST=\
	gcleanup_unix.cpp \
	gconvert_none.cpp \
	gconvert_unix.cpp \
	gdaemon_unix.cpp \
	gdatetime_unix.cpp \
	gdirectory_unix.cpp \
	genvironment_unix.cpp \
	gexecutablecommand_unix.cpp \
	gfile_unix.cpp \
	ghostname_unix.cpp \
	gidentity_unix.cpp \
	glogoutput_unix.cpp \
	gmsg_unix.cpp \
	gnewprocess_unix.cpp \
	gpam_linux.cpp \
	gprocess_unix.cpp

libglib_a_SOURCES = \
	galign.h \
	garg.h \
	garg.cpp \
	gassert.h \
	gbase64.h \
	gbase64.cpp \
	gbatchfile.h \
	gbatchfile.cpp \
	gcall.h \
	gcleanup.h \
	gcleanup_win32.cpp \
	gconvert.h \
	gconvert.cpp \
	gconvert_win32.cpp \
	gdaemon.h \
	gdaemon_win32.cpp \
	gdate.h \
	gdate.cpp \
	gdatetime.h \
	gdatetime.cpp \
	gdatetime_win32.cpp \
	gdef.h \
	gdirectory.h \
	gdirectory.cpp \
	gdirectory_win32.cpp \
	geightbit.h \
	genvironment.h \
	genvironment_win32.cpp \
	gexception.h \
	gexception.cpp \
	gexecutablecommand.h \
	gexecutablecommand.cpp \
	gexecutablecommand_win32.cpp \
	gfile.h \
	gfile.cpp \
	gfile_win32.cpp \
	ggetopt.h \
	ggetopt.cpp \
	ghash.h \
	ghash.cpp \
	ghashstate.h \
	ghostname.h \
	ghostname_win32.cpp \
	gidentity.h \
	gidentity_win32.cpp \
	glimits.h \
	glog.h \
	glog.cpp \
	glogoutput.h \
	glogoutput.cpp \
	glogoutput_win32.cpp \
	gstrmacros.h \
	gmapfile.h \
	gmapfile.cpp \
	gmd5.h \
	gmd5.cpp \
	gmsg.h \
	gmsg_win32.cpp \
	gnewprocess.h \
	gnewprocess_win32.cpp \
	goptionmap.h \
	goptionmap.cpp \
	goptionparser.h \
	goptionparser.cpp \
	goptions.h \
	goptions.cpp \
	goptionvalue.h \
	gpam.h \
	gpam_none.cpp \
	gpath.h \
	gpath.cpp \
	gpidfile.h \
	gpidfile.cpp \
	gprocess.h \
	gprocess_win32.cpp \
	greadwrite.h \
	greadwrite.cpp \
	groot.h \
	groot.cpp \
	gsignalsafe.h \
	gsleep.h \
	gslot.h \
	gslot.cpp \
	gstatemachine.h \
	gstr.h \
	gstr.cpp \
	gstrings.h \
	gtest.h \
	gtest.cpp \
	gthread.h \
	gthread.cpp \
	gtime.h \
	gtime.cpp \
	gxtext.h \
	gxtext.cpp

else

if GCONFIG_PAM
PAM_EXTRA_DIST = gpam_none.cpp
PAM_SOURCES = gpam_linux.cpp
else
PAM_EXTRA_DIST = gpam_linux.cpp
PAM_SOURCES = gpam_none.cpp
endif

if GCONFIG_ICONV
ICONV_EXTRA_DIST = gconvert_none.cpp gconvert_win32.cpp
ICONV_SOURCES = gconvert_unix.cpp
else
ICONV_EXTRA_DIST = gconvert_unix.cpp gconvert_win32.cpp
ICONV_SOURCES = gconvert_none.cpp
endif

EXTRA_DIST=\
	$(PAM_EXTRA_DIST) \
	$(ICONV_EXTRA_DIST) \
	gcleanup_win32.cpp \
	gdaemon_win32.cpp \
	gdatetime_win32.cpp \
	gdirectory_win32.cpp \
	genvironment_win32.cpp \
	gexecutablecommand_win32.cpp \
	gfile_win32.cpp \
	ghostname_win32.cpp \
	gidentity_win32.cpp \
	glogoutput_win32.cpp \
	gprocess_win32.cpp \
	gnewprocess_win32.cpp \
	gmsg_win32.cpp

libglib_a_SOURCES = \
	$(PAM_SOURCES) \
	$(ICONV_SOURCES) \
	garg.h \
	garg.cpp \
	gassert.h \
	gbase64.h \
	gbase64.cpp \
	gbatchfile.h \
	gbatchfile.cpp \
	gcall.h \
	gcleanup.h \
	gcleanup_unix.cpp \
	gconvert.h \
	gconvert.cpp \
	gdaemon.h \
	gdaemon_unix.cpp \
	gdate.h \
	gdate.cpp \
	gdatetime.h \
	gdatetime.cpp \
	gdatetime_unix.cpp \
	gdef.h \
	gdirectory.h \
	gdirectory.cpp \
	gdirectory_unix.cpp \
	genvironment.h \
	genvironment_unix.cpp \
	gexception.h \
	gexception.cpp \
	gexecutablecommand.h \
	gexecutablecommand.cpp \
	gexecutablecommand_unix.cpp \
	gfile.h \
	gfile.cpp \
	gfile_unix.cpp \
	ggetopt.h \
	ggetopt.cpp \
	ghash.h \
	ghash.cpp \
	ghostname.h \
	ghostname_unix.cpp \
	gidentity.h \
	gidentity_unix.cpp \
	glimits.h \
	glog.h \
	glog.cpp \
	glogoutput.h \
	glogoutput.cpp \
	glogoutput_unix.cpp \
	gmapfile.h \
	gmapfile.cpp \
	gmd5.h \
	gmd5.cpp \
	gmsg_unix.cpp \
	gmsg.h \
	gnewprocess.h \
	gnewprocess_unix.cpp \
	goptionmap.h \
	goptionmap.cpp \
	goptionparser.h \
	goptionparser.cpp \
	goptions.h \
	goptions.cpp \
	goptionvalue.h \
	gpam.h \
	gpath.h \
	gpath.cpp \
	gpidfile.h \
	gpidfile.cpp \
	gprocess.h \
	gprocess_unix.cpp \
	greadwrite.h \
	greadwrite.cpp \
	groot.h \
	groot.cpp \
	gsignalsafe.h \
	gsleep.h \
	gslot.h \
	gslot.cpp \
	gstatemachine.h \
	gstr.h \
	gstr.cpp \
	gstrings.h \
	gtest.h \
	gtest.cpp \
	gthread.h \
	gthread.cpp \
	gtime.h \
	gtime.cpp \
	gxtext.h \
	gxtext.cpp

endif
