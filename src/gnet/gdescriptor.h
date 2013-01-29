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
/// \file gdescriptor.h
///

#ifndef G_DESCRIPTOR_H
#define G_DESCRIPTOR_H

#include "gdef.h"
#include "gnet.h"
#include <iostream>

/// \namespace GNet
namespace GNet
{
	class Descriptor ;
}

/// \class GNet::Descriptor
/// A network file descriptor.
///
class GNet::Descriptor 
{
public:
	Descriptor() ;
		///< Default constructor.

	explicit Descriptor( SOCKET ) ;
		///< Constructor.

	bool valid() const ;
		///< Returns true if the descriptor is valid.

	static Descriptor invalid() ;
		///< Returns an invalid descriptor.

	SOCKET fd() const ;
		///< Returns the low-level descriptor.

	bool operator<( const Descriptor & other ) const ;
		///< Comparison operator.

private:
	SOCKET m_fd ;
} ;

/// \namespace GNet
namespace GNet
{
	inline
	std::ostream & operator<<( std::ostream & stream , const Descriptor & d )
	{
		stream << d.fd() ;
		return stream ;
	}
}

#endif

