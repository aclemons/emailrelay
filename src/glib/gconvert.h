//
// Copyright (C) 2001-2004 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gconvert.h
//

#ifndef G_CONVERT_H
#define G_CONVERT_H

#include "gdef.h"
#include "gexception.h"

namespace G
{

G_EXCEPTION( ConvertOverflow , "arithmetic overflow" ) ;

// Template function: G::Convert
// Description: Does arithmetic conversions with
// overflow checking.
// See also: boost::numeric_cast<>()
//
template <class Tout, class Tin>
inline
Tout Convert( const Tin & in )
{
	Tout out = in ;
	Tin copy = out ;
	if( in != copy )
		throw ConvertOverflow( std::ostringstream() << in ) ;
	return out ;
}

}

#endif

