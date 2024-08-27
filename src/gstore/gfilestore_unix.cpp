//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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
///
/// \file gfilestore_unix.cpp
///

#include "gdef.h"
#include "gfilestore.h"
#include "gpath.h"
#include "gstrmacros.h"

#ifndef G_SPOOLDIR
	#define G_SPOOLDIR
#endif

G::Path GStore::FileStore::defaultDirectory()
{
	std::string spooldir = G_STR(G_SPOOLDIR) ; // NOLINT readability-redundant-string-init
	if( spooldir.empty() )
		spooldir = "/var/spool/emailrelay" ;
	return { spooldir } ;
}

void GStore::FileStore::osinit()
{
	// no-op
}

