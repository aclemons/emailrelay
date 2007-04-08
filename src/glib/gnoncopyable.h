//
// Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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
///
/// \file gnoncopyable.h
///

#ifndef G_NONCOPYABLE_H
#define G_NONCOPYABLE_H

#include "gdef.h"

/// \namespace G
namespace G
{
	class noncopyable ;
}

/// \class G::noncopyable
/// A noncopyable base class (a la boost).
///
class G::noncopyable 
{
private:
	noncopyable( const noncopyable & ) ;
	void operator=( const noncopyable & ) ;
public:
	noncopyable() {}
} ;

#endif

