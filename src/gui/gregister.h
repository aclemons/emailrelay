//
// Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gregister.h
///

#ifndef G_REGISTER_H__
#define G_REGISTER_H__

#include "gdef.h"
#include "gpath.h"

/// \class GRegister
/// A class for registering a server executable with the o/s in some way.
///
class GRegister 
{
public:
	static void server( const G::Path & path ) ;
		///< Registers the given server executable. Throws on error.

private:
	GRegister() ;
} ;

#endif
