//
// Copyright (C) 2001-2006 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// passwd-fastbuild.cpp
//
#define G_NO_DEBUG
#define G_NO_ASSERT
#define _BSD_SOURCE
#include "gdef.h"
#include "gnet.h"
#include "gsmtp.h"
#ifdef G_UNIX
#include "garg_unix.cpp"
#include "gfs_unix.cpp"
#else
#include "garg_win32.cpp"
#include "gfs_win32.cpp"
#endif
#include "garg.cpp"
#include "gexception.cpp"
#include "md5.cpp"
#include "gmd5_native.cpp"
#include "gpath.cpp"
#include "gstr.cpp"
#include "legal.cpp"
#include "passwd.cpp"
