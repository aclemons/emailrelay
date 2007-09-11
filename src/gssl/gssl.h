//
// Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gssl.h
///

#ifndef G_SSL_H
#define G_SSL_H

#include "gdef.h"
#include <string>

/// \namespace GSsl
namespace GSsl
{
	class Library ;
	class Protocol ;
	class LibraryImp ;
	class ProtocolImp ;
}

/// \class GSsl::Protocol
/// An SSL protocol class. The protocol object
/// is tied to a particular socket file descriptor.
///
class GSsl::Protocol 
{
public:
	typedef size_t size_type ;
	typedef ssize_t ssize_type ;
	enum Result { Result_ok , Result_read , Result_write , Result_error } ;
	typedef void (*LogFn)( const std::string & ) ;

	explicit Protocol( const Library & ) ;
		///< Constructor.

	Protocol( const Library & , LogFn , bool hexdump = defaultHexdump() ) ;
		///< Constructor.

	~Protocol() ;
		///< Destructor.

	Result connect( int fd ) ;
		///< Starts the protocol actively.

	Result accept( int fd ) ;
		///< Starts the protocol passively.

	Result stop() ;
		///< Initiates the protocol shutdown.

	Result read( char * buffer , size_type buffer_size_in , ssize_type & data_size_out ) ;
		///< Reads data into the supplied buffer.
		///<
		///< Note that a retry will need the same buffer 
		///< pointer value.

	Result write( const char * buffer , size_type data_size_in , ssize_type & data_size_out ) ;
		///< Writes data.
		///<
		///< Note that a retry will need the same buffer
		///< pointer value.

	static std::string str( Result result ) ;
		///< Converts a result enumeration into a 
		///< printable string. Used in logging and 
		///< diagnostics.

	static bool defaultHexdump() ;
		///< Returns a default value for the constructor parameter.

private:
	Protocol( const Protocol & ) ;
	void operator=( const Protocol & ) ;

private:
	ProtocolImp * m_imp ;
} ;

/// \class GSsl::Library
/// A RAII class for initialising the underlying ssl library.
///
class GSsl::Library 
{
public:
	Library() ;
		///< Constructor. Initialises the underlying ssl library.

	Library( bool active , const std::string & pem_file ) ;
		///< Constructor. Initialises the underlying ssl library
		///< or not. The pem file is needed if acting as a
		///< server.

	~Library() ;
		///< Destructor. Cleans up the underlying ssl library.

	static Library * instance() ;
		///< Returns a pointer to a library object, if any.

	bool enabled() const ;
		///< Returns true if this is a real and enabled 
		///< ssl library.

	static std::string credit( const std::string & prefix , const std::string & eol , const std::string & final ) ;
		///< Returns a credit string.

private:
	Library( const Library & ) ;
	void operator=( const Library & ) ;

private:
	friend class GSsl::Protocol ;
	static Library * m_this ;
	LibraryImp * m_imp ;
} ;

#endif
