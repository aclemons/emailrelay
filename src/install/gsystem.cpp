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
// gsystem.cpp
//

#include "gsystem.h"

#ifdef _WIN32
G::Path GSystem::install()
{
	return "c:\\program files\\emailrelay" ;
}
G::Path GSystem::spool()
{
	return "c:\\windows\\spool\\emailrelay" ;
}
G::Path GSystem::config()
{
	return "c:\\windows" ;
}
#else
G::Path GSystem::install()
{
	return "/usr/local/emailrelay" ;
}
G::Path GSystem::spool()
{
	return "/var/spool/emailrelay" ;
}
G::Path GSystem::config()
{
	return "/etc" ;
}
#endif
