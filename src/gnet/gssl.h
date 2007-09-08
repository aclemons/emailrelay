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
#include <exception>
#include <vector>

/// \namespace GSsl
namespace GSsl
{
	class Library ;
	class Context ;
	class Protocol ;
	class Error ;

	class LibraryImp ;
	class ContextImp ;
	class ProtocolImp ;
}

/// \class GSsl::Context
/// An openssl context wrapper.
///
class GSsl::Context 
{
public:
	Context() ;
		///< Constructor.

	~Context() ;
		///< Destructor.

	const ContextImp * p( int ) const ;
		///< Private accessor for the Protocol class.

private:
	ContextImp * m_imp ;

private:
	Context( const Context & ) ;
	void operator=( const Context & ) ;
	void setQuietShutdown() ;
} ;

/// \class GSsl::Error
/// An exception class.
///
class GSsl::Error : public std::exception 
{
public:
	explicit Error( const std::string & ) ;
		///< Constructor.

	Error( const std::string & , unsigned long ) ;
		///< Constructor.

	virtual ~Error() throw() ;
		///< Destructor.

	virtual const char * what() const throw() ;
		///< Override from std::exception.

private:
	std::string m_what ;
} ;

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

	explicit Protocol( const Context & ) ;
		///< Constructor.

	Protocol( const Context & , LogFn , bool hexdump = false ) ;
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

private:
	LogFn m_log_fn ;
	ProtocolImp * m_imp ;
	bool m_fd_set ;

private:
	friend class GSsl::LibraryImp ;
	LogFn log() const ;

private:
	Protocol( const Protocol & ) ;
	void operator=( const Protocol & ) ;
	int error( const char * , int ) const ;
	void set( int ) ;
	Result connect() ;
	Result accept() ;
	static Result convert( int ) ;
} ;

/// \class GSsl::Library
/// A RAII class for initialising the underlying ssl library.
///
class GSsl::Library 
{
public:
	Library() ;
		///< Constructor. Initialises the underlying ssl library.

	~Library() ;
		///< Destructor. Cleans up the underlying ssl library.

	static void clearErrors() ;
		///< Clears the error stack.

	static std::string str( Protocol::Result result ) ;
		///< Converts a protocol result enumeration
		///< into a printable string. Used in 
		///< logging and diagnostics.

private:
	Library( const Library & ) ;
	void operator=( const Library & ) ;
} ;


#endif
