//
// Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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

#ifndef G_NET_DESCRIPTOR__H
#define G_NET_DESCRIPTOR__H

#include "gdef.h"
#include <iostream>

namespace GNet
{
	class Descriptor ;
}

/// \class GNet::Descriptor
/// A class that encapsulates a network socket file descriptor and
/// an associated windows event handle.
///
class GNet::Descriptor
{
public:
	Descriptor() ;
		///< Default constructor.

	explicit Descriptor( SOCKET , HANDLE = 0 ) ;
		///< Constructor.

	bool valid() const ;
		///< Returns true if the socket part is valid, ignoring
		///< the handle.

	static Descriptor invalid() ;
		///< Returns a descriptor with an invalid socket part and
		///< a zero handle.

	SOCKET fd() const ;
		///< Returns the socket part.

	HANDLE h() const ;
		///< Returns the handle part.

	bool operator==( const Descriptor & other ) const ;
		///< Comparison operator.

	bool operator!=( const Descriptor & other ) const ;
		///< Comparison operator.

	bool operator<( const Descriptor & other ) const ;
		///< Comparison operator.

	void streamOut( std::ostream & ) const ;
		///< Used by op<<().

private:
	SOCKET m_fd ;
	HANDLE m_handle ;
} ;

inline
GNet::Descriptor::Descriptor( SOCKET fd , HANDLE h ) :
	m_fd(fd) ,
	m_handle(h)
{
}

inline
SOCKET GNet::Descriptor::fd() const
{
	return m_fd ;
}

inline
bool GNet::Descriptor::operator==( const Descriptor & other ) const
{
	return m_fd == other.m_fd && m_handle == other.m_handle ;
}

inline
bool GNet::Descriptor::operator!=( const Descriptor & other ) const
{
	return !(*this == other) ;
}

inline
bool GNet::Descriptor::operator<( const Descriptor & other ) const
{
	return m_fd == other.m_fd ? ( m_handle < other.m_handle ) : ( m_fd < other.m_fd ) ;
}

inline
GNet::Descriptor GNet::Descriptor::invalid()
{
	return Descriptor() ;
}

namespace GNet
{
	inline
	std::ostream & operator<<( std::ostream & stream , const Descriptor & d )
	{
		d.streamOut( stream ) ;
		return stream ;
	}
}

#endif
