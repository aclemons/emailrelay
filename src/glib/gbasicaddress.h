//
// Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gbasicaddress.h
///

#ifndef G_BASIC_ADDRESS_H
#define G_BASIC_ADDRESS_H

#include "gdef.h"

namespace G
{
	class BasicAddress ;
}

//| \class G::BasicAddress
/// A structure that holds a network address as a string with no
/// dependency on any low-level network library.
/// \see GNet::Address
///
class G::BasicAddress
{
public:
	explicit BasicAddress( const std::string & s = {} ) ;
		///< Constructor.

	std::string displayString() const ;
		///< Returns a printable string that represents the transport
		///< address.

private:
	std::string m_display_string ;
} ;

inline
G::BasicAddress::BasicAddress( const std::string & s ) :
	m_display_string(s)
{
}

inline
std::string G::BasicAddress::displayString() const
{
	return m_display_string ;
}

#endif
