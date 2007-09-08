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
//
// gssl.cpp
//


#include "gdef.h"
#include "gssl.h"
#if HAVE_OPENSSL
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <vector>
#include <cassert>
#include <iomanip>
#include <sstream>

class GSsl::ProtocolImp 
{
} ;

class GSsl::ContextImp 
{
} ;

// 

class GSsl::LibraryImp 
{
public:
	static SSL_CTX * ctx( ContextImp * ) ;
	static SSL_CTX * ctx( const ContextImp * ) ;
	static ContextImp * imp( SSL_CTX * ) ;
	static SSL * ssl( ProtocolImp * ) ;
	static ProtocolImp * imp( SSL * ) ;
	static unsigned long getError() ;
	static void loghex( void (*fn)(const std::string&) , const char * , const std::string & ) ;
	static void callback( int , int , int , const void * , size_t , SSL * , void * p ) ;

private:
	LibraryImp() ;
} ;

SSL_CTX * GSsl::LibraryImp::ctx( ContextImp * p )
{
	return reinterpret_cast<SSL_CTX*>(p) ;
}

SSL_CTX * GSsl::LibraryImp::ctx( const ContextImp * p )
{
	return const_cast<SSL_CTX*>(reinterpret_cast<const SSL_CTX*>(p)) ;
}

GSsl::ContextImp * GSsl::LibraryImp::imp( SSL_CTX * p )
{
	return reinterpret_cast<ContextImp*>(p) ;
}

SSL * GSsl::LibraryImp::ssl( ProtocolImp * p )
{
	return reinterpret_cast<SSL*>(p) ;
}

GSsl::ProtocolImp * GSsl::LibraryImp::imp( SSL * p )
{
	return reinterpret_cast<ProtocolImp*>(p) ;
}

unsigned long GSsl::LibraryImp::getError()
{
	return ERR_get_error() ;
}

void GSsl::LibraryImp::loghex( void (*fn)(const std::string&) , const char * prefix , const std::string & in )
{
	std::string line ;
	unsigned int i = 0 ;
	for( std::string::const_iterator p = in.begin() ; p != in.end() ; ++p , i++ )
	{
		std::ostringstream ss ;
		if( line.empty() )
		{
			ss.width(6) ;
			ss.fill('0') ;
			ss << std::hex << i << ": " ;
		}
		ss.width(2) ;
		ss.fill('0') ;
		ss << std::hex << (static_cast<unsigned int>(*p) & 0xff) << " " ;
		line.append( ss.str() ) ;

		if( i > 0 && ((i+1)%16) == 0 )
		{
			(*fn)( std::string(prefix) + line ) ;
			line.erase() ;
		}
	}
}

void GSsl::LibraryImp::callback( int write , int v , int type , const void * buffer , size_t n , SSL * , void * p )
{
	Protocol * protocol_p = reinterpret_cast<Protocol*>(p) ;
	if( protocol_p->log() != NULL ) 
	{
		// build the whole pdu, including the header
		unsigned int version_ = static_cast<unsigned int>(v) ;
		unsigned int version_lo = version_ & 0xff ;
		unsigned int version_hi = ( version_ >> 8 ) & 0xff ;
		unsigned int n_ = static_cast<unsigned int>(n) ;
		unsigned int length_lo = n_ & 0xff ;
		unsigned int length_hi = ( n_ >> 8 ) & 0xff ;
		std::string data( 1U , static_cast<char>(type) ) ;
		data.append( 1U , static_cast<char>(version_hi) ) ;
		data.append( 1U , static_cast<char>(version_lo) ) ;
		data.append( 1U , static_cast<char>(length_hi) ) ;
		data.append( 1U , static_cast<char>(length_lo) ) ;
		data.append( std::string(reinterpret_cast<const char*>(buffer),n) ) ;

		loghex( protocol_p->log() , write?"ssl-tx>>: ":"ssl-rx<<: " , data ) ;
	}
}

//

GSsl::Library::Library()
{
	SSL_load_error_strings() ;
	SSL_library_init() ;
	// TODO -- add entropy here -- for now assume /dev/urandom
}

GSsl::Library::~Library()
{
	ERR_free_strings() ;
}

void GSsl::Library::clearErrors()
{
	for( int i = 0 ; LibraryImp::getError() && i < 10000 ; i++ )
		;
}

std::string GSsl::Library::str( Protocol::Result result )
{
	if( result == GSsl::Protocol::Result_ok ) return "Result_ok" ;
	if( result == GSsl::Protocol::Result_read ) return "Result_read" ;
	if( result == GSsl::Protocol::Result_write ) return "Result_write" ;
	if( result == GSsl::Protocol::Result_error ) return "Result_error" ;
	return "Result_undefined" ;
}

//

GSsl::Context::Context()
{
	m_imp = LibraryImp::imp( SSL_CTX_new(TLSv1_method()) ) ;
	if( m_imp == NULL )
		throw Error( "SSL_CTX_new" , LibraryImp::getError() ) ;

	setQuietShutdown() ;
}

GSsl::Context::~Context()
{
	SSL_CTX_free( LibraryImp::ctx(m_imp) ) ;
}

const GSsl::ContextImp * GSsl::Context::p( int ) const
{
	return m_imp ;
}

void GSsl::Context::setQuietShutdown()
{
	SSL_CTX_set_quiet_shutdown( LibraryImp::ctx(m_imp) , 1 ) ;
}

//

GSsl::Error::Error( const std::string & s ) :
	m_what(std::string()+"ssl error: "+s)
{
}

GSsl::Error::Error( const std::string & s , unsigned long e ) :
	m_what(std::string()+"ssl error: "+s)
{
	std::vector<char> v( 300 ) ;
	ERR_error_string_n( e , &v[0] , v.size() ) ;
	std::string reason( &v[0] , v.size() ) ;
	reason = std::string( reason.c_str() ) ; // (lazy)
	m_what.append( std::string() + ": [" + reason + "]" ) ;
}

GSsl::Error::~Error() throw ()
{
}

const char * GSsl::Error::what() const throw ()
{
	return m_what.c_str() ;
}

// 

GSsl::Protocol::Protocol( const Context & c ) :
	m_log_fn(NULL) ,
	m_fd_set(false)
{
	m_imp = LibraryImp::imp( SSL_new(LibraryImp::ctx(c.p(0))) ) ;
	if( m_imp == NULL )
		throw Error( "SSL_new" , LibraryImp::getError() ) ;
}

GSsl::Protocol::Protocol( const Context & c , LogFn log , bool hexdump ) :
	m_log_fn(log) ,
	m_fd_set(false)
{
	m_imp = LibraryImp::imp( SSL_new(LibraryImp::ctx(c.p(0))) ) ;
	if( m_imp == NULL )
		throw Error( "SSL_new" , LibraryImp::getError() ) ;

	if( hexdump )
	{
		SSL_set_msg_callback( LibraryImp::ssl(m_imp) , LibraryImp::callback ) ;
		SSL_set_msg_callback_arg( LibraryImp::ssl(m_imp) , this ) ;
	}
}

GSsl::Protocol::~Protocol()
{
	SSL_free( LibraryImp::ssl(m_imp) ) ;
}

GSsl::Protocol::LogFn GSsl::Protocol::log() const
{
	return m_log_fn ;
}

int GSsl::Protocol::error( const char * op , int rc ) const
{
	int e = SSL_get_error( LibraryImp::ssl(m_imp) , rc ) ;

	if( m_log_fn != NULL )
	{
		std::ostringstream ss ;
		ss << "ssl error: " << op << ": rc=" << rc << ": error " << e << " => " << Library::str(convert(e)) ;
		(*m_log_fn)( ss.str() ) ;
		unsigned long ee = 0 ;
		for(;;)
		{
			ee = ERR_get_error() ;
			if( ee == 0 ) break ;
			Error eee( op , ee ) ;
			(*m_log_fn)( std::string() + eee.what() ) ;
		}
	}

	return e ;
}

GSsl::Protocol::Result GSsl::Protocol::convert( int e )
{
	if( e == SSL_ERROR_WANT_READ ) return Result_read ;
	if( e == SSL_ERROR_WANT_WRITE ) return Result_write ;
	return Result_error ;
}

GSsl::Protocol::Result GSsl::Protocol::connect( int fd )
{
	set( fd ) ;
	return connect() ;
}

GSsl::Protocol::Result GSsl::Protocol::accept( int fd )
{
	set( fd ) ;
	return accept() ;
}

void GSsl::Protocol::set( int fd )
{
	if( !m_fd_set )
	{
		int rc = SSL_set_fd( LibraryImp::ssl(m_imp) , fd ) ;
		if( rc == 0 )
			throw Error( "SSL_set_fd" , LibraryImp::getError() ) ;
		m_fd_set = true ;
	}
}

GSsl::Protocol::Result GSsl::Protocol::connect()
{
	Library::clearErrors() ;
	int rc = SSL_connect( LibraryImp::ssl(m_imp) ) ;
	if( rc >= 1 )
	{
		return Result_ok ;
	}
	else if( rc == 0 )
	{
		return convert(error("SSL_connect",rc)) ;
	}
	else // rc < 0
	{
		return convert(error("SSL_connect",rc)) ;
	}
}

GSsl::Protocol::Result GSsl::Protocol::accept()
{
	Library::clearErrors() ;
	int rc = SSL_accept( LibraryImp::ssl(m_imp) ) ;
	if( rc >= 1 )
	{
		return Result_ok ;
	}
	else if( rc == 0 )
	{
		return convert(error("SSL_accept",rc)) ;
	}
	else // rc < 0
	{
		return convert(error("SSL_accept",rc)) ;
	}
}

GSsl::Protocol::Result GSsl::Protocol::stop()
{
	int rc = SSL_shutdown( LibraryImp::ssl(m_imp) ) ;
	return rc == 1 ? Result_ok : Result_error ; // since quiet shutdown
}

GSsl::Protocol::Result GSsl::Protocol::read( char * buffer , size_type buffer_size , ssize_type & read_size )
{
	read_size = 0 ;
	Library::clearErrors() ;
	
	int rc = SSL_read( LibraryImp::ssl(m_imp) , buffer , buffer_size ) ;
	if( rc > 0 )
	{
		read_size = static_cast<ssize_type>(rc) ;
		return Result_ok ;
	}
	else if( rc == 0 )
	{
		return convert(error("SSL_read",rc)) ;
	}
	else // rc < 0 
	{
		return convert(error("SSL_read",rc)) ;
	}
}

GSsl::Protocol::Result GSsl::Protocol::write( const char * buffer , size_type size_in , ssize_type & size_out )
{
	size_out = 0 ;

	Library::clearErrors() ;
	int rc = SSL_write( LibraryImp::ssl(m_imp) , buffer , size_in ) ;
	if( rc > 0 )
	{
		size_out = static_cast<ssize_type>(rc) ;
		return Result_ok ;
	}
	else if( rc == 0 )
	{
		return convert(error("SSL_write",rc)) ;
	}
	else // rc < 0
	{
		return convert(error("SSL_write",rc)) ;
	}
}

#endif
/// \file gssl.cpp
