//
// Copyright (C) 2001-2003 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gmemory.h
//

#ifndef G_MEMORY_H
#define G_MEMORY_H

#include "gdef.h"
#include <memory>

// Template function: operator<<=
// Description: A fix for the problem of resetting an auto_ptr<> 
// portably. MSVC6.0 & GCC 2.91 do not have a reset() method, 
// and GCC 2.95 has a non-const assignment operators.
//
// Usage:
/// #include <memory>
/// #include "gmemory.h"
/// {
///   std::auto_ptr<Foo> ptr ;
///   for( int i = 0 ; i < 10 ; i++ )
///   {
///      ptr <<= new Foo ;
///      if( ptr->fn() )
///         eatFoo( ptr->release() ) ;
///   }
/// }
//
template <class T>
void operator<<=( std::auto_ptr<T> & ap , T * p )
{
	std::auto_ptr<T> temp( p ) ;
	ap = temp ;
}

// Template function: operator<<=
// Description: A version for null-pointer constants.
//
template <class T>
void operator<<=( std::auto_ptr<T> & ap , int /* null_pointer */ )
{
	T * p = 0 ;
	ap <<= p ;
}

#endif
