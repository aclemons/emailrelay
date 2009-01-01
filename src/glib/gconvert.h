//
// Copyright (C) 2001-2009 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gconvert.h
///

#ifndef G_CONVERT_H
#define G_CONVERT_H

#include "gdef.h"
#include "gexception.h"
#include <sstream>

/// \namespace G
namespace G
{

G_EXCEPTION( ConvertOverflow , "arithmetic overflow" ) ;

/// 
/// Does arithmetic conversions with
/// overflow checking.
/// \see boost::numeric_cast<>()
///
template <typename Tout, typename Tin>
Tout Convert( const Tin & in )
{
	Tout out = in ;
	Tin copy = out ;
	if( in != copy )
	{
		std::ostringstream ss ;
		ss << in ;
		throw ConvertOverflow( ss.str() ) ;
	}
	return out ;
}

}

#endif

