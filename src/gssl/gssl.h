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
/// \file gssl.h
///

#ifndef G_SSL_H
#define G_SSL_H

#include "gdef.h"
#include <string>
#include <utility>

/// \namespace GSsl
namespace GSsl
{
	class Library ;
	class Protocol ;
	class LibraryImp ;
	class ProtocolImp ;
}

/// \class GSsl::Protocol
/// An SSL protocol class. The protocol object is
/// associated with a particular socket file descriptor by the 
/// connect() and accept() calls.
///
/// The protocol is half-duplex in the sense that it is not 
/// possible to read() data while a write() is incomplete. 
/// (Nor is it allowed to issue a second write() while the 
/// first write() is still incomplete.) Client code will 
/// typically need at least two states: a reading state and 
/// a writing state. In each state the file descriptor read 
/// events and write events will be handled identically; in 
/// the reading state by a call to Protocol::read(), and in 
/// the writing state by a call to Protocol::write().
///
/// All logging is done indirectly through a logging function
/// pointer; the first parameter is the logging level which is
/// 0 for hex dump data, 1 for verbose debug messages and 2 for
/// more important errors and warnings. Some implemetations
/// do not log anything useful.
///
class GSsl::Protocol 
{
public:
	typedef size_t size_type ;
	typedef ssize_t ssize_type ;
	enum Result { Result_ok , Result_read , Result_write , Result_error , Result_more } ;
	typedef void (*LogFn)( int , const std::string & ) ;

	explicit Protocol( const Library & ) ;
		///< Constructor.

	Protocol( const Library & , LogFn ) ;
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
		///< Reads user data into the supplied buffer. 
		///<
		///< Returns Result_read if there is not enough transport data 
		///< to complete the internal SSL data packet. In this case the 
		///< file descriptor should remain in the select() read list and 
		///< the Protocol::read() should be retried using the same parameters
		///< when the file descriptor is ready to be read.
		///<
		///< Returns Result_write if the SSL layer tried to write to the
		///< file descriptor and had flow control asserted. In this case
		///< the file descriptor should be added to the select() write
		///< list and the Protocol::read() should be retried using the
		///< same parameters when the file descriptor is ready to be 
		///< written.
		///<
		///< Returns Result_ok if the internal SSL data packet is complete
		///< and it has been completely deposited in the supplied buffer.
		///<
		///< Returns Result_more if the internal SSL data packet is complete
		///< and the supplied buffer was too small to take it all.
		///<
		///< Returns Result_error if the transport connnection was lost 
		///< or if the SSL session was shut down by the peer or on error.

	Result write( const char * buffer , size_type data_size_in , ssize_type & data_size_out ) ;
		///< Writes user data.
		///<
		///< Returns Result_ok if fully sent.
		///<
		///< Returns Result_read if the SSL layer needs more transport
		///< data (eg. for a renegotiation). The write() should be repeated
		///< using the same parameters on the file descriptor's next 
		///< readable event.
		///<
		///< Returns Result_write if the SSL layer was blocked in 
		///< writing transport data. The write() should be repeated
		///< using the same parameters on the file descriptor's next
		///< writable event.
		///<
		///< Returns Result_error if the transport connnection was lost 
		///< or if the SSL session was shut down by the peer or on error.

	static std::string str( Result result ) ;
		///< Converts a result enumeration into a printable string. 
		///< Used in logging and diagnostics.

	std::pair<std::string,bool> peerCertificate( int format = 0 ) ;
		///< Returns the peer certificate and a verified flag.
		///< The default format of the certificate is printable 
		///< with embedded newlines but otherwise unspecified.

private:
	Protocol( const Protocol & ) ; // not implemented
	void operator=( const Protocol & ) ; // not implemented

private:
	ProtocolImp * m_imp ;
} ;

/// \class GSsl::Library
/// A RAII class for initialising the underlying ssl library.
///
class GSsl::Library 
{
public:
	typedef Protocol::LogFn LogFn ;

	Library() ;
		///< Constructor. Initialises the underlying ssl library
		///< for use as a client.

	Library( bool active , const std::string & pem_file , unsigned int flags , LogFn = NULL ) ;
		///< Constructor. Initialises the underlying ssl library
		///< or not (if the first parameter is false). The pem 
		///< file is required if acting as a server. The flags
		///< should default to zero; their meaning are opaque at 
		///< this interface.

	~Library() ;
		///< Destructor. Cleans up the underlying ssl library.

	static Library * instance() ;
		///< Returns a pointer to a library object, if any.

	bool enabled( bool for_serving = false ) const ;
		///< Returns true if this is a real and enabled 
		///< ssl library.

	static std::string credit( const std::string & prefix , const std::string & eol , const std::string & final ) ;
		///< Returns a credit string.

private:
	Library( const Library & ) ; // not implemented
	void operator=( const Library & ) ; // not implemented
	const LibraryImp & imp() const ;

private:
	friend class GSsl::Protocol ;
	static Library * m_this ;
	LibraryImp * m_imp ;
} ;

#endif
