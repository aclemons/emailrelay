//
// Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ===
//
// glibsources.cpp
//
// These are the source files that are shared between the gui and the
// other executables, so this file can be compiled with Qt-friendly
// compiler flags without affecting the rest of the build.
//
#include "gdef.h"
#include "garg.cpp"
#include "gbase64.cpp"
#include "gbatchfile.cpp"
#include "gconvert.cpp"
#include "gdate.cpp"
#include "gdatetime.cpp"
#include "gdirectory.cpp"
#include "gexception.cpp"
#include "gexecutablecommand.cpp"
#include "gfile.cpp"
#include "ggetopt.cpp"
#include "ghash.cpp"
#include "glog.cpp"
#include "glogoutput.cpp"
#include "gmapfile.cpp"
#include "gmd5.cpp"
#include "goptionparser.cpp"
#include "goptions.cpp"
#include "goptionmap.cpp"
#include "gpath.cpp"
#include "gstr.cpp"
#include "gtest.cpp"
#include "gtime.cpp"
#include "gxtext.cpp"
#include "options.cpp"
#include "service_install.cpp"
#include "service_remove.cpp"
#ifdef G_WINDOWS
#include "gconvert_win32.cpp"
#include "gdatetime_win32.cpp"
#include "gdirectory_win32.cpp"
#include "genvironment_win32.cpp"
#include "gexecutablecommand_win32.cpp"
#include "gfile_win32.cpp"
#include "gidentity_win32.cpp"
#include "glogoutput_win32.cpp"
#include "gnewprocess_win32.cpp"
#include "gprocess_win32.cpp"
#else
#include "gconvert_unix.cpp"
#include "gdatetime_unix.cpp"
#include "gdirectory_unix.cpp"
#include "genvironment_unix.cpp"
#include "gexecutablecommand_unix.cpp"
#include "gfile_unix.cpp"
#include "gidentity_unix.cpp"
#include "glogoutput_unix.cpp"
#include "gnewprocess_unix.cpp"
#include "gprocess_unix.cpp"
#endif
/// \file glibsources.cpp
