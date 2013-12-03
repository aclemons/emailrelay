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
//
// gssl_openssl.cpp
//

#include "gdef.h"
#include "gssl.h"
#include "gtest.h"
#include "gstr.h"
#include "gpath.h"
#include "gexception.h"
#include "glog.h"
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <exception>
#include <vector>
#include <cassert>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <utility>

// debugging...
//  * network logging
//     $ sudo tcpdump -s 0 -n -i eth0 -X tcp port 587
//  * emailrelay smtp proxy to gmail
//     $ emailrelay --forward-to smtp.gmail.com:587 ...
//  * openssl smtp client to gmail
//     $ openssl s_client -tls1 -msg -debug -starttls smtp -crlf -connect smtp.gmail.com:587
//  * certificate
//     $ openssl req -x509 -nodes -days 365 -subj "/C=US/ST=Oregon/L=Portland/CN=eight.local" 
//           -newkey rsa:1024 -keyout test.cert  -out test.cert
//     $ cp test.cert /etc/ssl/certs/
//     $ cd /etc/ssl/certs && ln -s `openssl x509 -noout -hash -in test.cert`.0
//  * openssl server (without smtp)
//     $ openssl s_server -accept 10025 -cert /etc/ssl/certs/test.pem -debug -msg -tls1
//

namespace GSsl
{
	class Context ;
	class Error ;
	class Certificate ;
}

/// \class GSsl::Context
/// An openssl context wrapper.
/// 
class GSsl::Context 
{
public:
	explicit Context( const std::string & pem_file = std::string() , unsigned int flags = 0U ) ;
	~Context() ;
	SSL_CTX * p() const ;

private:
	Context( const Context & ) ;
	void operator=( const Context & ) ;
	void init( const std::string & pem_file ) ;
	static void check( int , const char * ) ;

private:
	SSL_CTX * m_ssl_ctx ;
} ;

/// \class GSsl::Certificate
/// An openssl X509 RAII class.
/// 
class GSsl::Certificate 
{
public:
	explicit Certificate( X509* ) ;
	~Certificate() ;
	std::string str() const ;

private:
	Certificate( const Certificate & ) ;
	void operator=( const Certificate & ) ;

private:
	X509 * m_p ;
} ;

/// \class GSsl::LibraryImp
/// A private pimple class used by GSsl::Library.
/// 
class GSsl::LibraryImp 
{
public:
	typedef Library::LogFn LogFn ;
	explicit LibraryImp( const std::string & pem_file = std::string(), unsigned int flags = 0U, LogFn log_fn = NULL ) ;
	~LibraryImp() ;
	Context & ctx() const ;
	std::string pem() const ;
	LogFn logFn() const ;
	unsigned int flags() const ;

private:
	LibraryImp( const LibraryImp & ) ;
	void operator=( const LibraryImp & ) ;

private:
	Context * m_context ;
	std::string m_pem_file ;
	unsigned int m_flags ;
	LogFn m_log_fn ;
} ;

/// \class GSsl::ProtocolImp
/// A private pimple class used by GSsl::Protocol.
/// 
class GSsl::ProtocolImp 
{
public:
	typedef Protocol::Result Result ;
	typedef Protocol::LogFn LogFn ;
	typedef Protocol::size_type size_type ;
	typedef Protocol::ssize_type ssize_type ;

	explicit ProtocolImp( const Context & c , unsigned int flags ) ;
	ProtocolImp( const Context & c , unsigned int flags , LogFn log ) ;
	~ProtocolImp() ;
	Result connect( int ) ;
	Result accept( int ) ;
	Result stop() ;
	Result read( char * buffer , size_type buffer_size , ssize_type & read_size ) ;
	Result write( const char * buffer , size_type size_in , ssize_type & size_out ) ;
	std::pair<std::string,bool> peerCertificate() ;

private:
	ProtocolImp( const ProtocolImp & ) ;
	void operator=( const ProtocolImp & ) ;
	int error( const char * , int ) const ;
	void set( int ) ;
	Result connect() ;
	Result accept() ;
	static Result convert( int ) ;
	static void clearErrors() ;

private:
	SSL * m_ssl ;
	unsigned int m_flags ;
	LogFn m_log_fn ;
	bool m_fd_set ;
} ;

/// \class GSsl::Error
/// A private exception class used by ssl classes.
/// 
class GSsl::Error : public std::exception 
{
public:
	explicit Error( const std::string & ) ;
	Error( const std::string & , unsigned long ) ;
	virtual ~Error() throw() ;
	virtual const char * what() const throw() ;

private:
	std::string m_what ;
} ;

//

GSsl::LibraryImp::LibraryImp( const std::string & pem_file , unsigned int flags , LogFn log_fn ) :
	m_context(NULL) ,
	m_pem_file(pem_file) ,
	m_flags(flags) ,
	m_log_fn(log_fn)
{
	SSL_load_error_strings() ;
	SSL_library_init() ;

	// we probably don't need extra entropy but make a token effort to 
	// find some - quote: "openssl automatically queries EGD when [...]
	// the status is checked via RAND_status() for the first time if [a] 
	// socket is located at /var/run/edg-pool ..."
	//
	G_IGNORE_RETURN(int) RAND_status() ;

	m_context = new Context( pem_file , flags ) ;
}

GSsl::LibraryImp::~LibraryImp()
{
	delete m_context ;
	ERR_free_strings() ;
	RAND_cleanup() ;
}

unsigned int GSsl::LibraryImp::flags() const
{
	return m_flags ;
}

GSsl::Library::LogFn GSsl::LibraryImp::logFn() const
{
	return m_log_fn ;
}

GSsl::Context & GSsl::LibraryImp::ctx() const
{
	return *m_context ;
}

std::string GSsl::LibraryImp::pem() const
{
	return m_pem_file ;
}

//

GSsl::Library * GSsl::Library::m_this = NULL ;

GSsl::Library::Library() :
	m_imp(NULL)
{
	if( m_this == NULL )
		m_this = this ;
	m_imp = new LibraryImp ;
}

GSsl::Library::Library( bool active , const std::string & pem_file , unsigned int flags , LogFn log_fn ) :
	m_imp(NULL)
{
	if( m_this == NULL )
		m_this = this ;
	if( active )
		m_imp = new LibraryImp( pem_file , flags , log_fn ) ;
}

GSsl::Library::~Library()
{
	delete m_imp ;
	if( m_this == NULL )
		m_this = NULL ;
}

GSsl::Library * GSsl::Library::instance()
{
	return m_this ;
}

bool GSsl::Library::enabled( bool for_server ) const
{
	return m_imp != NULL && ( !for_server || !m_imp->pem().empty() ) ;
}

const GSsl::LibraryImp & GSsl::Library::imp() const
{
	if( m_imp == NULL )
		throw G::Exception( "internal error: no ssl library instance" ) ;
	return *m_imp ;
}

std::string GSsl::Library::credit( const std::string & prefix , const std::string & eol , const std::string & final )
{
	std::ostringstream ss ;
	ss
		<< prefix << "This product includes software developed by the OpenSSL Project" << eol
		<< prefix << "for use in the OpenSSL Toolkit (http://www.openssl.org/)" << eol
		<< final ;
	return ss.str() ;
}

//

namespace
{
	int verify_callback_always_pass( int , X509_STORE_CTX * )
	{
		return 1 ;
	}
}

GSsl::Context::Context( const std::string & pem_file , unsigned int flags )
{
	if( (flags&3U) == 2U )
		m_ssl_ctx = SSL_CTX_new(SSLv23_method()) ;
	else if( (flags&3U) == 3U )
		m_ssl_ctx = SSL_CTX_new(SSLv3_method()) ;
	else
		m_ssl_ctx = SSL_CTX_new(TLSv1_method()) ;

	if( m_ssl_ctx == NULL )
		throw Error( "SSL_CTX_new" , ERR_get_error() ) ;

	if( flags&4U )
	{
		// ask for certificates but dont actually verify them
		SSL_CTX_set_verify( m_ssl_ctx , SSL_VERIFY_PEER , verify_callback_always_pass ) ;
	}
	if( (flags&8U) && !pem_file.empty() )
	{
		// ask for certificates and make sure they verify
		SSL_CTX_set_verify( m_ssl_ctx , SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT , NULL ) ;
		check( SSL_CTX_load_verify_locations( m_ssl_ctx , NULL , 
			G::Path(pem_file).dirname().str().c_str() ) , "load_verify_locations" ) ;
	}

	init( pem_file ) ;
}

GSsl::Context::~Context()
{
	SSL_CTX_free( m_ssl_ctx ) ;
}

SSL_CTX * GSsl::Context::p() const
{
	return m_ssl_ctx ;
}

void GSsl::Context::init( const std::string & pem_file )
{
	G_DEBUG( "GSsl::Context::init: [" << pem_file << "]" ) ;
	SSL_CTX_set_quiet_shutdown( m_ssl_ctx , 1 ) ;
	if( !pem_file.empty() )
	{
		check( SSL_CTX_use_certificate_chain_file(m_ssl_ctx,pem_file.c_str()) , "use_certificate_chain_file" ) ;
		check( SSL_CTX_use_RSAPrivateKey_file(m_ssl_ctx,pem_file.c_str(),SSL_FILETYPE_PEM) , "use_RSAPrivateKey_file" );
		check( SSL_CTX_set_cipher_list(m_ssl_ctx,"DEFAULT") , "set_cipher_list" ) ;
	}
}

void GSsl::Context::check( int rc , const char * op )
{
	if( rc != 1 )
		throw Error( std::string() + "SSL_CTX_" + op ) ;
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
	reason = std::string( reason.c_str() ) ; // no nuls
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

GSsl::Protocol::Protocol( const Library & library ) :
	m_imp( new ProtocolImp(library.imp().ctx(),library.imp().flags(),library.imp().logFn()) )
{
}

GSsl::Protocol::Protocol( const Library & library , LogFn log_fn ) :
	m_imp( new ProtocolImp(library.imp().ctx(),library.imp().flags(),log_fn) )
{
}

GSsl::Protocol::~Protocol()
{
	delete m_imp ;
}

std::pair<std::string,bool> GSsl::Protocol::peerCertificate( int )
{
	return m_imp->peerCertificate() ;
}

std::string GSsl::Protocol::str( Protocol::Result result )
{
	if( result == Result_ok ) return "Result_ok" ;
	if( result == Result_read ) return "Result_read" ;
	if( result == Result_write ) return "Result_write" ;
	if( result == Result_error ) return "Result_error" ;
	return "Result_undefined" ;
}

GSsl::Protocol::Result GSsl::Protocol::connect( int fd )
{
	return m_imp->connect( fd ) ;
}

GSsl::Protocol::Result GSsl::Protocol::accept( int fd )
{
	return m_imp->accept( fd ) ;
}

GSsl::Protocol::Result GSsl::Protocol::stop()
{
	return m_imp->stop() ;
}

GSsl::Protocol::Result GSsl::Protocol::read( char * buffer , size_type buffer_size_in , ssize_type & data_size_out )
{
	return m_imp->read( buffer , buffer_size_in , data_size_out ) ;
}

GSsl::Protocol::Result GSsl::Protocol::write( const char * buffer , size_type data_size_in , 
	ssize_type & data_size_out )
{
	return m_imp->write( buffer , data_size_in , data_size_out ) ;
}

//

GSsl::ProtocolImp::ProtocolImp( const Context & c , unsigned int flags ) :
	m_ssl(NULL) ,
	m_flags(flags) ,
	m_log_fn(NULL) ,
	m_fd_set(false)
{
	m_ssl = SSL_new( c.p() ) ;
	if( m_ssl == NULL )
		throw Error( "SSL_new" , ERR_get_error() ) ;
}

GSsl::ProtocolImp::ProtocolImp( const Context & c , unsigned int flags , LogFn log_fn ) :
	m_ssl(NULL) ,
	m_flags(flags) ,
	m_log_fn(log_fn) ,
	m_fd_set(false)
{
	m_ssl = SSL_new( c.p() ) ;
	if( m_ssl == NULL )
		throw Error( "SSL_new" , ERR_get_error() ) ;
}

GSsl::ProtocolImp::~ProtocolImp()
{
	SSL_free( m_ssl ) ;
}

void GSsl::ProtocolImp::clearErrors()
{
	for( int i = 0 ; ERR_get_error() && i < 10000 ; i++ )
		;
}

int GSsl::ProtocolImp::error( const char * op , int rc ) const
{
	int e = SSL_get_error( m_ssl , rc ) ;

	if( m_log_fn != NULL )
	{
		std::ostringstream ss ;
		ss << "ssl error: " << op << ": rc=" << rc << ": error " << e << " => " << Protocol::str(convert(e)) ;
		(*m_log_fn)( 1 , ss.str() ) ;
		unsigned long ee = 0 ;
		for( int i = 2 ; i < 10000 ; i++ )
		{
			ee = ERR_get_error() ;
			if( ee == 0 ) break ;
			Error eee( op , ee ) ;
			(*m_log_fn)( 2 , std::string() + eee.what() ) ;
		}
	}

	return e ;
}

GSsl::Protocol::Result GSsl::ProtocolImp::convert( int e )
{
	if( e == SSL_ERROR_WANT_READ ) return Protocol::Result_read ;
	if( e == SSL_ERROR_WANT_WRITE ) return Protocol::Result_write ;
	return Protocol::Result_error ;
}

GSsl::Protocol::Result GSsl::ProtocolImp::connect( int fd )
{
	set( fd ) ;
	return connect() ;
}

GSsl::Protocol::Result GSsl::ProtocolImp::accept( int fd )
{
	set( fd ) ;
	return accept() ;
}

void GSsl::ProtocolImp::set( int fd )
{
	if( !m_fd_set )
	{
		int rc = SSL_set_fd( m_ssl , fd ) ;
		if( rc == 0 )
			throw Error( "SSL_set_fd" , ERR_get_error() ) ;

		if( G::Test::enabled("log-ssl-bio") ) // log bio activity directly to stderr
		{
			BIO_set_callback( SSL_get_rbio(m_ssl) , BIO_debug_callback ) ;
			BIO_set_callback( SSL_get_wbio(m_ssl) , BIO_debug_callback ) ;
		}

		m_fd_set = true ;
	}
}

GSsl::Protocol::Result GSsl::ProtocolImp::connect()
{
	clearErrors() ;
	int rc = SSL_connect( m_ssl ) ;
	if( rc >= 1 )
	{
		return Protocol::Result_ok ;
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

GSsl::Protocol::Result GSsl::ProtocolImp::accept()
{
	clearErrors() ;
	int rc = SSL_accept( m_ssl ) ;
	if( rc >= 1 )
	{
		return Protocol::Result_ok ;
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

GSsl::Protocol::Result GSsl::ProtocolImp::stop()
{
	int rc = SSL_shutdown( m_ssl ) ;
	return rc == 1 ? Protocol::Result_ok : Protocol::Result_error ; // since quiet shutdown
}

GSsl::Protocol::Result GSsl::ProtocolImp::read( char * buffer , size_type buffer_size_in , ssize_type & read_size )
{
	read_size = 0 ;

	clearErrors() ;
	int buffer_size = static_cast<int>(buffer_size_in) ;
	int rc = SSL_read( m_ssl , buffer , buffer_size ) ;
	if( rc > 0 )
	{
		read_size = static_cast<ssize_type>(rc) ;
		return SSL_pending(m_ssl) ? Protocol::Result_more : Protocol::Result_ok ;
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

GSsl::Protocol::Result GSsl::ProtocolImp::write( const char * buffer , size_type size_in , ssize_type & size_out )
{
	size_out = 0 ;

	clearErrors() ;
	int size = static_cast<int>(size_in) ;
	int rc = SSL_write( m_ssl , buffer , size ) ;
	if( rc > 0 )
	{
		size_out = static_cast<ssize_type>(rc) ;
		return Protocol::Result_ok ;
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

std::pair<std::string,bool> GSsl::ProtocolImp::peerCertificate()
{
	std::pair<std::string,bool> result ;
	result.first = Certificate(SSL_get_peer_certificate(m_ssl)).str() ;
	result.second = false ; // since we return a bogus result from our callback
	return result ;
}

// ==

GSsl::Certificate::Certificate( X509 * p ) :
	m_p(p)
{
}

GSsl::Certificate::~Certificate()
{
	if( m_p != NULL )
		X509_free( m_p ) ;
}

std::string GSsl::Certificate::str() const
{
	if( m_p == NULL ) return std::string() ;
	BIO * bio = BIO_new( BIO_s_mem() ) ;
	int rc = PEM_write_bio_X509( bio , m_p ) ;
	if( !rc ) return std::string() ;
	BUF_MEM * mem = NULL ;
	BIO_get_mem_ptr( bio , &mem ) ;
	size_t n = mem ? static_cast<size_t>(mem->length) : 0U ;
	const char * p = mem ? mem->data : NULL ;
	std::string data = p&&n ? std::string(p,n) : std::string() ;
	BIO_free( bio ) ;

	// sanitise to be strictly printable with embedded newlines
	std::string result = G::Str::printable( data , '\0' ) ;
	G::Str::replaceAll( result , std::string(1U,'\0')+"n" , "\n" ) ;
	G::Str::replaceAll( result , std::string(1U,'\0') , "\\" ) ;
	return result ;
}

