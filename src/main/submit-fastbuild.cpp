//
// Copyright (C) 2001-2005 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later
// version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
// ===
//
// submit-fastbuild.cpp
//
#define _BSD_SOURCE
#include "gdef.h"
#include "gnet.h"
#include "gsmtp.h"
#ifdef G_UNIX
#include <grp.h>
#include "garg_unix.cpp"
#include "gdatetime_unix.cpp"
#include "gdescriptor_unix.cpp"
#include "gdirectory_unix.cpp"
#include "geventloop_unix.cpp"
#include "gfile_unix.cpp"
#include "gfs_unix.cpp"
#include "gidentity_unix.cpp"
#include "glocal_unix.cpp"
#include "glogoutput_unix.cpp"
#include "gmessagestore_unix.cpp"
#include "gprocess_unix.cpp"
#include "gresolve_unix.cpp"
#include "gsocket_unix.cpp"
#else
#include "gdatetime_win32.cpp"
#include "gdescriptor_win32.cpp"
#include "geventloop_win32.cpp"
#include "gidentity_win32.cpp"
#include "glocal_win32.cpp"
#include "gprocess_win32.cpp"
#include "gresolve_win32.cpp"
#include "gsocket_win32.cpp"
#endif
#include "gaddress_ipv4.cpp"
#include "garg.cpp"
#include "gdatetime.cpp"
#include "gdirectory.cpp"
#include "geventhandler.cpp"
#include "geventloop.cpp"
#include "gexception.cpp"
#include "gexe.cpp"
#include "gfile.cpp"
#include "gfilestore.cpp"
#include "ggetopt.cpp"
#include "glocal.cpp"
#include "glog.cpp"
#include "glogoutput.cpp"
#include "md5.cpp"
#include "gmd5_native.cpp"
#include "gmessagestore.cpp"
#include "gnewfile.cpp"
#include "gnewmessage.cpp"
#include "gpath.cpp"
#include "gprocessor.cpp"
#include "gresolve.cpp"
#include "gresolve_ipv4.cpp"
#include "groot.cpp"
#include "gslot.cpp"
#include "gsocket.cpp"
#include "gstoredfile.cpp"
#include "gstoredmessage.cpp"
#include "gstr.cpp"
#include "gtimer.cpp"
#include "gverifier.cpp"
#include "gxtext.cpp"
#include "legal.cpp"
#include "submit.cpp"
