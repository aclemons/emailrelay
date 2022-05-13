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

AM_CPPFLAGS =

noinst_LIBRARIES = libglib.a

if GCONFIG_WINDOWS

EXTRA_DIST=\
	gcleanup_unix.cpp \
	gdaemon_unix.cpp \
	gdirectory_unix.cpp \
	genvironment_unix.cpp \
	gexecutablecommand_unix.cpp \
	gfile_unix.cpp \
	ggettext_unix.cpp \
	ggettext_none.cpp \
	ghostname_unix.cpp \
	gidentity_unix.cpp \
	glogoutput_unix.cpp \
	gmsg_unix.cpp \
	gnewprocess_unix.cpp \
	gpam_linux.cpp \
	gprocess_unix.cpp

libglib_a_SOURCES = \
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
	gdef.h \
	gdirectory.h \
	gdirectory.cpp \
	gdirectory_win32.cpp \
	genvironment.h \
	genvironment.cpp \
	genvironment_win32.cpp \
	gexception.h \
	gexception.cpp \
	gexecutablecommand.h \
	gexecutablecommand.cpp \
	gexecutablecommand_win32.cpp \
	gfile.h \
	gfile.cpp \
	gfile_win32.cpp \
	gformat.h \
	gformat.cpp \
	ggetopt.h \
	ggetopt.cpp \
	ggettext.h \
	ggettext_win32.cpp \
	ghash.h \
	ghash.cpp \
	ghashstate.h \
	ghostname.h \
	ghostname_win32.cpp \
	gidentity.h \
	gidentity_win32.cpp \
	gimembuf.h \
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
	gomembuf.h \
	goption.h \
	goption.cpp \
	goptionmap.h \
	goptionmap.cpp \
	goptionparser.h \
	goptionparser.cpp \
	goptions.h \
	goptions.cpp \
	goptionsoutput.h \
	goptionsoutput.cpp \
	goptionvalue.h \
	gpam.h \
	gpam_none.cpp \
	gpath.h \
	gpath.cpp \
	gpidfile.h \
	gpidfile.cpp \
	gprocess.h \
	gprocess_win32.cpp \
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
	gstatemachine.cpp \
	gstr.h \
	gstr.cpp \
	gstringfield.h \
	gstringtoken.h \
	gstringarray.h \
	gstringmap.h \
	gstringwrap.cpp \
	gstringwrap.h \
	gstringview.h \
	gtest.h \
	gtest.cpp \
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

if GCONFIG_GETTEXT
GETTEXT_EXTRA_DIST = ggettext_win32.cpp ggettext_none.cpp
GETTEXT_SOURCES = ggettext_unix.cpp
else
GETTEXT_EXTRA_DIST = ggettext_win32.cpp ggettext_unix.cpp
GETTEXT_SOURCES = ggettext_none.cpp
endif

EXTRA_DIST=\
	gcleanup_win32.cpp \
	gconvert.cpp \
	gconvert_win32.cpp \
	gdaemon_win32.cpp \
	gdirectory_win32.cpp \
	genvironment_win32.cpp \
	gexecutablecommand_win32.cpp \
	gfile_win32.cpp \
	$(GETTEXT_EXTRA_DIST) \
	ghostname_win32.cpp \
	gidentity_win32.cpp \
	glogoutput_win32.cpp \
	$(PAM_EXTRA_DIST) \
	gprocess_win32.cpp \
	gnewprocess_win32.cpp \
	gmsg_win32.cpp

libglib_a_SOURCES = \
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
	gcleanup_unix.cpp \
	gconvert.h \
	gdaemon.h \
	gdaemon_unix.cpp \
	gdate.h \
	gdate.cpp \
	gdatetime.h \
	gdatetime.cpp \
	gdef.h \
	gdirectory.h \
	gdirectory.cpp \
	gdirectory_unix.cpp \
	genvironment.h \
	genvironment.cpp \
	genvironment_unix.cpp \
	gexception.h \
	gexception.cpp \
	gexecutablecommand.h \
	gexecutablecommand.cpp \
	gexecutablecommand_unix.cpp \
	gfile.h \
	gfile.cpp \
	gfile_unix.cpp \
	gformat.h \
	gformat.cpp \
	ggetopt.h \
	ggetopt.cpp \
	ggettext.h \
	$(GETTEXT_SOURCES) \
	ghash.h \
	ghash.cpp \
	ghostname.h \
	ghostname_unix.cpp \
	gidentity.h \
	gidentity_unix.cpp \
	gimembuf.h \
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
	gomembuf.h \
	goption.h \
	goption.cpp \
	goptionmap.h \
	goptionmap.cpp \
	goptionparser.h \
	goptionparser.cpp \
	goptions.h \
	goptions.cpp \
	goptionsoutput.h \
	goptionsoutput.cpp \
	goptionvalue.h \
	gpam.h \
	$(PAM_SOURCES) \
	gpath.h \
	gpath.cpp \
	gpidfile.h \
	gpidfile.cpp \
	gprocess.h \
	gprocess_unix.cpp \
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
	gstatemachine.cpp \
	gstr.h \
	gstr.cpp \
	gstringfield.h \
	gstringtoken.h \
	gstringarray.h \
	gstringmap.h \
	gstringwrap.cpp \
	gstringwrap.h \
	gstringview.h \
	gtest.h \
	gtest.cpp \
	gthread.cpp \
	gtime.h \
	gtime.cpp \
	gxtext.h \
	gxtext.cpp

endif
