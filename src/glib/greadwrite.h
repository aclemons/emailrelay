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
/// \file greadwrite.h
///

#ifndef G_READ_WRITE_H
#define G_READ_WRITE_H

#include "gdef.h"

namespace G
{
	class ReadWrite ;
}

//| \class G::ReadWrite
/// An abstract interface for reading and writing from a non-blocking
/// i/o channel.
///
/// Eg:
/// \code
/// ssize_t rc = s.read( buffer , buffer.size() ) ;
/// if( rc == 0 ) throw Disconnected() ;
/// else if( rc < 0 && !s.eWouldBlock() ) throw ReadError() ;
/// else if( rc > 0 ) got_some( buffer , rc ) ;
/// else /*nothing-to-do*/ ;
///
/// ssize_t rc = s.write( buffer , buffer.size() )
/// if( rc < 0 && !s.eWouldBlock() ) throw Disconnected() ;
/// else if( rc < 0 || rc < buffer.size() ) sent_some( rc < 0 ? 0 : rc ) ;
/// else sent_all() ;
/// \endcode
///
class G::ReadWrite
{
public:
	using size_type = std::size_t ;
	using ssize_type = ssize_t ;

	virtual ssize_type read( char * buffer , size_type buffer_length ) = 0 ;
		///< Reads data. Returns 0 if the connection has been lost.
		///< Returns -1 on error or if there is nothing to read (in
		///< which case eWouldBlock() returns true).

	virtual ssize_type write( const char * buf , size_type len ) = 0 ;
		///< Sends data. Returns the amount of data sent.
		///<
		///< If this method returns -1 then use eWouldBlock() to
		///< determine whether there was a flow control problem;
		///< if it returns -1 and eWouldBlock() returns false then
		///< the connection is lost. If it returns less than the
		///< requested length then eWouldBlock() should not be
		///< used.

	virtual bool eWouldBlock() const = 0 ;
		///< See read() and write().

	virtual SOCKET fd() const noexcept = 0 ;
		///< Returns the file descriptor.

	virtual ~ReadWrite() = default ;
		///< Destructor.
} ;

#endif
