//
// Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gssl_mbedtls.cpp
//

#include "gdef.h"
#include "gssl.h"
#include "gssl_mbedtls.h"
#include "ghashstate.h"
#include "gpath.h"
#include "groot.h"
#include "gtest.h"
#include "gstr.h"
#include "gfile.h"
#include "glog.h"
#include "gassert.h"
#include <mbedtls/ssl.h>
#include <mbedtls/ssl_ciphersuites.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/certs.h>
#include <mbedtls/error.h>
#include <mbedtls/version.h>
#include <mbedtls/pem.h>
#include <mbedtls/base64.h>
#include <mbedtls/debug.h>
#include <sstream>
#include <fstream>
#include <exception>
#include <iomanip>

namespace
{
	template <typename T>
	struct Cleanup
	{
		explicit Cleanup( void (*fn)(T*) ) : m_p(nullptr) , m_fn(fn) {}
		~Cleanup() { reset() ; }
		void reset( T * p = nullptr ) { if(m_p) m_fn(m_p) ; m_p = p ; }
		void release() { m_p = nullptr ; }
		T * m_p ;
		void (*m_fn)(T*) ;
	} ;
}

GSsl::MbedTls::LibraryImp::LibraryImp( G::StringArray & library_config , Library::LogFn log_fn , bool verbose ) :
	m_log_fn(log_fn) ,
	m_config(library_config)
{
	mbedtls_debug_set_threshold( verbose ? 3 : 1 ) ; // "Messages that have a level over the threshold value are ignored."
}

GSsl::MbedTls::LibraryImp::~LibraryImp()
{
}

const GSsl::MbedTls::Rng & GSsl::MbedTls::LibraryImp::rng() const
{
	return m_rng ;
}

void GSsl::MbedTls::LibraryImp::addProfile( const std::string & profile_name , bool is_server_profile ,
	const std::string & key_file , const std::string & cert_file , const std::string & ca_file ,
	const std::string & default_peer_certificate_name , const std::string & default_peer_host_name ,
	const std::string & profile_config )
{
	shared_ptr<ProfileImp> profile_ptr(
		new ProfileImp(*this,is_server_profile,key_file,cert_file,ca_file,
			default_peer_certificate_name,default_peer_host_name,profile_config) ) ;
	m_profile_map.insert( Map::value_type(profile_name,profile_ptr) ) ;
}

bool GSsl::MbedTls::LibraryImp::hasProfile( const std::string & profile_name ) const
{
	Map::const_iterator p = m_profile_map.find( profile_name ) ;
	return p != m_profile_map.end() ;
}

const GSsl::Profile & GSsl::MbedTls::LibraryImp::profile( const std::string & profile_name ) const
{
	Map::const_iterator p = m_profile_map.find( profile_name ) ;
	if( p == m_profile_map.end() ) throw Error( "no such profile: [" + profile_name + "]" ) ;
	return *(*p).second.get() ;
}

GSsl::Library::LogFn GSsl::MbedTls::LibraryImp::log() const
{
	return m_log_fn ;
}

std::string GSsl::MbedTls::LibraryImp::sid()
{
	std::vector<char> buffer( 100U ) ; // "at least 18"
	G_ASSERT( buffer.size() >= 18U ) ;
	mbedtls_version_get_string_full( &buffer[0] ) ;
	buffer[buffer.size()-1U] = '\0' ;
	return G::Str::printable( std::string(&buffer[0]) ) ;
}

std::string GSsl::MbedTls::LibraryImp::id() const
{
	return sid() ;
}

GSsl::MbedTls::Config GSsl::MbedTls::LibraryImp::config() const
{
	return m_config ;
}

std::string GSsl::MbedTls::LibraryImp::credit( const std::string & prefix , const std::string & eol , const std::string & eot )
{
	std::ostringstream ss ;
	ss
		<< prefix << "mbed TLS: Copy" << "right (C) 2006-2016, ARM Limited" << eol
		<< eot ;
	return ss.str() ;
}

G::StringArray GSsl::MbedTls::LibraryImp::digesters( bool ) const
{
	G::StringArray result ;
	result.push_back( "MD5" ) ;
	result.push_back( "SHA1" ) ;
	result.push_back( "SHA256" ) ;
	return result ;
}

GSsl::Digester GSsl::MbedTls::LibraryImp::digester( const std::string & hash_type , const std::string & state ) const
{
	return Digester( new MbedTls::DigesterImp(hash_type,state) ) ;
}

// ==

GSsl::MbedTls::Config::Config( G::StringArray & config ) :
	m_noverify(consume(config,"noverify")),
	m_min(-1),
	m_max(-1)
{
	static const int SSL_v3 = MBEDTLS_SSL_MINOR_VERSION_0 ;
	static const int TLS_v1_0 = MBEDTLS_SSL_MINOR_VERSION_1 ;
	static const int TLS_v1_1 = MBEDTLS_SSL_MINOR_VERSION_2 ;
	static const int TLS_v1_2 = MBEDTLS_SSL_MINOR_VERSION_3 ;

	G_ASSERT( SSL_v3 >= 0 ) ;
	G_ASSERT( TLS_v1_0 >= 0 ) ;
	G_ASSERT( TLS_v1_1 >= 0 ) ;
	G_ASSERT( TLS_v1_2 >= 0 ) ;

	if( consume(config,"sslv3") ) m_min = SSL_v3 ;
	if( consume(config,"tlsv1.0") ) m_min = TLS_v1_0 ;
	if( consume(config,"tlsv1.1") ) m_min = TLS_v1_1 ;
	if( consume(config,"tlsv1.2") ) m_min = TLS_v1_2 ;

	if( consume(config,"-sslv3") ) m_max = SSL_v3 ;
	if( consume(config,"-tlsv1.0") ) m_max = TLS_v1_0 ;
	if( consume(config,"-tlsv1.1") ) m_max = TLS_v1_1 ;
	if( consume(config,"-tlsv1.2") ) m_max = TLS_v1_2 ;
}

int GSsl::MbedTls::Config::min_() const
{
	return m_min ;
}

int GSsl::MbedTls::Config::max_() const
{
	return m_max ;
}

bool GSsl::MbedTls::Config::noverify() const
{
	return m_noverify ;
}

bool GSsl::MbedTls::Config::consume( G::StringArray & list , const std::string & item )
{
	return LibraryImp::consume( list , item ) ;
}

// ==

GSsl::MbedTls::DigesterImp::DigesterImp( const std::string & hash_type , const std::string & state ) :
	m_hash_type(G::Str::upper(hash_type))
{
	if( m_hash_type == "MD5" )
	{
		mbedtls_md5_init( &m_md5 ) ;
		if( state.empty() )
			mbedtls_md5_starts( &m_md5 ) ;
		else
			G::HashState<16,uint32_t,uint32_t>::decode( state , m_md5.state , m_md5.total[0] ) ;
	}
	else if( m_hash_type == "SHA1" )
	{
		mbedtls_sha1_init( &m_sha1 ) ;
		if( state.empty() )
			mbedtls_sha1_starts( &m_sha1 ) ;
		else
			G::HashState<20,uint32_t,uint32_t>::decode( state , m_sha1.state , m_sha1.total[0] ) ;
	}
	else if( m_hash_type == "SHA256" )
	{
		mbedtls_sha256_init( &m_sha256 ) ;
		if( state.empty() )
			mbedtls_sha256_starts( &m_sha256 , 0 ) ;
		else
			G::HashState<32,uint32_t,uint32_t>::decode( state , m_sha256.state , m_sha256.total[0] ) ;
	}
}

GSsl::MbedTls::DigesterImp::~DigesterImp()
{
	if( m_hash_type == "MD5" )
		mbedtls_md5_free( &m_md5 ) ;
	else if( m_hash_type == "SHA1" )
		mbedtls_sha1_free( &m_sha1 ) ;
	else if( m_hash_type == "SHA256" )
		mbedtls_sha256_free( &m_sha256 ) ;
}

void GSsl::MbedTls::DigesterImp::add( const std::string & s )
{
	if( m_hash_type == "MD5" )
		mbedtls_md5_update( &m_md5 , reinterpret_cast<const unsigned char*>(s.data()) , s.size() ) ;
	else if( m_hash_type == "SHA1" )
		mbedtls_sha1_update( &m_sha1 , reinterpret_cast<const unsigned char*>(s.data()) , s.size() ) ;
	else if( m_hash_type == "SHA256" )
		mbedtls_sha256_update( &m_sha256 , reinterpret_cast<const unsigned char*>(s.data()) , s.size() ) ;
}

std::string GSsl::MbedTls::DigesterImp::value()
{
	if( m_hash_type == "MD5" )
	{
		unsigned char buffer[16] ;
		mbedtls_md5_finish( &m_md5 , buffer ) ;
		return std::string( reinterpret_cast<const char*>(buffer) , sizeof(buffer) ) ;
	}
	else if( m_hash_type == "SHA1" )
	{
		unsigned char buffer[20] ;
		mbedtls_sha1_finish( &m_sha1 , buffer ) ;
		return std::string( reinterpret_cast<const char*>(buffer) , sizeof(buffer) ) ;
	}
	else if( m_hash_type == "SHA256" )
	{
		unsigned char buffer[32] ;
		mbedtls_sha256_finish( &m_sha256 , buffer ) ;
		return std::string( reinterpret_cast<const char*>(buffer) , sizeof(buffer) ) ;
	}
	else
	{
		return std::string() ;
	}
}

std::string GSsl::MbedTls::DigesterImp::state()
{
	if( m_hash_type == "MD5" )
		return G::HashState<16,uint32_t,uint32_t>::encode( m_md5.state , m_md5.total[0] ) ;
	else if( m_hash_type == "SHA1" )
		return G::HashState<20,uint32_t,uint32_t>::encode( m_sha1.state , m_sha1.total[0] ) ;
	else if( m_hash_type == "SHA256" )
		return G::HashState<32,uint32_t,uint32_t>::encode( m_sha256.state , m_sha256.total[0] ) ;
	else
		return std::string() ;
}

size_t GSsl::MbedTls::DigesterImp::blocksize() const
{
	return 64U ;
}

size_t GSsl::MbedTls::DigesterImp::valuesize() const
{
	if( m_hash_type == "MD5" )
		return 16U ;
	else if( m_hash_type == "SHA1" )
		return 20U ;
	else if( m_hash_type == "SHA256" )
		return 32U ;
	else
		return 0U ;
}

size_t GSsl::MbedTls::DigesterImp::statesize() const
{
	return valuesize() + 4U ;
}

// ==

GSsl::MbedTls::ProfileImp::ProfileImp( const LibraryImp & library_imp , bool is_server_profile ,
	const std::string & key_file , const std::string & cert_file , const std::string & ca_path ,
	const std::string & default_peer_certificate_name , const std::string & default_peer_host_name ,
	const std::string & profile_config ) :
		m_library_imp(library_imp) ,
		m_default_peer_certificate_name(default_peer_certificate_name) ,
		m_default_peer_host_name(default_peer_host_name)
{
	Cleanup<mbedtls_ssl_config> cleanup( mbedtls_ssl_config_free ) ;

	// use library config, or override with profile config
	Config extra_config = library_imp.config() ;
	if( !profile_config.empty() )
	{
		G::StringArray profile_config_list = G::Str::splitIntoTokens( profile_config , "," ) ;
		extra_config = Config( profile_config_list ) ;
		if( !profile_config_list.empty() )
			G_WARNING( "GSsl::MbedTls::ProfileImp::ctor: tls-config: tls " << (is_server_profile?"server":"client")
				<< " profile configuration ignored: [" << G::Str::join(",",profile_config_list) << "]" ) ;
	}

	// initialise the mbedtls_ssl_config structure
	{
		static mbedtls_ssl_config config_zero ;
		m_config = config_zero ;
		mbedtls_ssl_config_init( &m_config ) ;
		cleanup.reset( &m_config ) ;
		int rc = mbedtls_ssl_config_defaults( &m_config ,
			is_server_profile ? MBEDTLS_SSL_IS_SERVER : MBEDTLS_SSL_IS_CLIENT ,  // see also mbedtls_ssl_conf_endpoint()
			MBEDTLS_SSL_TRANSPORT_STREAM ,
			MBEDTLS_SSL_PRESET_DEFAULT ) ;
		if( rc ) throw Error( "mbedtls_ssl_config_defaults" , rc ) ;
	}

	// load up the certificate and private key ready for mbedtls_ssl_conf_own_cert()
	if( !key_file.empty() )
		m_pk.load( key_file ) ;
	if( !cert_file.empty() )
		m_certificate.load( cert_file ) ;

	// identify our certificate/private-key combination
	if( m_certificate.loaded() )
	{
		int rc = mbedtls_ssl_conf_own_cert( &m_config , m_certificate.ptr() , m_pk.ptr() ) ;
		if( rc != 0 )
			throw Error( "mbedtls_ssl_conf_own_cert" , rc ) ;
	}

	// configure verification
	{
		std::string ca_path_default = "/etc/ssl/certs/ca-certificates.crt" ; // see "man update-ca-certificates"
		if( ca_path.empty() )
		{
			// verify if possible, but continue on failure - see mbedtls_ssl_get_verify_result()
			mbedtls_ssl_conf_authmode( &m_config , MBEDTLS_SSL_VERIFY_OPTIONAL ) ;
		}
		else if( ca_path == "<none>" )
		{
			// dont verify
			mbedtls_ssl_conf_authmode( &m_config , MBEDTLS_SSL_VERIFY_NONE ) ;
		}
		else if( ca_path == "<default>" )
		{
			// verify against the default ca database
			m_ca_list.load( ca_path_default ) ;
			bool no_verify = extra_config.noverify() ;
			mbedtls_ssl_conf_ca_chain( &m_config , m_ca_list.ptr() , crl() ) ;
			mbedtls_ssl_conf_authmode( &m_config , no_verify ? MBEDTLS_SSL_VERIFY_OPTIONAL : MBEDTLS_SSL_VERIFY_REQUIRED ) ;
		}
		else
		{
			// verify against the given ca database
			m_ca_list.load( ca_path ) ;
			bool no_verify = extra_config.noverify() ;
			mbedtls_ssl_conf_ca_chain( &m_config , m_ca_list.ptr() , crl() ) ;
			mbedtls_ssl_conf_authmode( &m_config , no_verify ? MBEDTLS_SSL_VERIFY_OPTIONAL : MBEDTLS_SSL_VERIFY_REQUIRED ) ;
		}
	}

	// configure protocol version
	{
		if( extra_config.min_() >= 0 )
			mbedtls_ssl_conf_min_version( &m_config , MBEDTLS_SSL_MAJOR_VERSION_3 , extra_config.min_() ) ;
		if( extra_config.max_() >= 0 )
			mbedtls_ssl_conf_max_version( &m_config , MBEDTLS_SSL_MAJOR_VERSION_3 , extra_config.max_() ) ;
	}

	// hooks
	{
		mbedtls_ssl_conf_rng( &m_config , mbedtls_ctr_drbg_random , m_library_imp.rng().ptr() ) ;
		mbedtls_ssl_conf_dbg( &m_config , onDebug , this ) ;
	}

	// other configuration
	{
		mbedtls_ssl_conf_renegotiation( &m_config , MBEDTLS_SSL_RENEGOTIATION_DISABLED ) ;
	}
	cleanup.release() ;
}

GSsl::MbedTls::ProfileImp::~ProfileImp()
{
	mbedtls_ssl_config_free( &m_config ) ;
}

GSsl::ProtocolImpBase * GSsl::MbedTls::ProfileImp::newProtocol( const std::string & peer_certificate_name ,
	const std::string & peer_host_name ) const
{
	return
		new MbedTls::ProtocolImp( *this ,
			peer_certificate_name.empty()?defaultPeerCertificateName():peer_certificate_name ,
			peer_host_name.empty()?defaultPeerHostName():peer_host_name ) ;
}

mbedtls_x509_crl * GSsl::MbedTls::ProfileImp::crl() const
{
	// TODO certificate revocation list
	return nullptr ;
}

const mbedtls_ssl_config * GSsl::MbedTls::ProfileImp::config() const
{
	return &m_config ;
}

void GSsl::MbedTls::ProfileImp::onDebug( void * This , int level_in , const char * file , int line , const char * message )
{
	try
	{
		reinterpret_cast<ProfileImp*>(This)->doDebug( level_in , file , line , message ) ;
	}
	catch(...) // callback from c code
	{
	}
}

void GSsl::MbedTls::ProfileImp::doDebug( int level_in , const char * file , int line , const char * message )
{
	// in practice even level 0 messages are too noisy, so discard them all :(
	level_in = 4 ;

	// map from 'level_in' mbedtls levels to GSsl::Library::LogFn levels...
	// 4 -> <discarded>
	// 3 -> logAt(1) (verbose-debug)
	// 2 -> logAt(1) (verbose-debug)
	// 1 -> logAt(3) (errors-and-warnings)
	// 0 -> logAt(3) (errors-and-warnings)
	// ... with this code doing its own logAt(2) (useful-information)
	// see also mbedtls_debug_set_threshold() in LibraryImp::ctor()
	//
	int level_out = level_in >= 4 ? 0 : ( level_in >= 2 ? 1 : 3 ) ;
	if( m_library_imp.log() && level_out )
	{
		std::ostringstream ss ;
		G::Path path( file ? std::string(file) : std::string() ) ;
		ss << path.basename() << "(" << line << "): " << (message?message:"") ;
		logAt( level_out , ss.str() ) ;
	}
}

void GSsl::MbedTls::ProfileImp::logAt( int level_out , std::string s ) const
{
	Library::LogFn log_fn = m_library_imp.log() ;
	if( log_fn )
	{
		s = G::Str::printable( G::Str::trimmed(s,G::Str::ws()) ) ;
		if( !s.empty() )
			(*log_fn)( level_out , s ) ;
	}
}

const std::string & GSsl::MbedTls::ProfileImp::defaultPeerCertificateName() const
{
	return m_default_peer_certificate_name ;
}

const std::string & GSsl::MbedTls::ProfileImp::defaultPeerHostName() const
{
	return m_default_peer_host_name ;
}

// ==

GSsl::MbedTls::ProtocolImp::ProtocolImp( const ProfileImp & profile , const std::string & required_peer_certificate_name ,
	const std::string & target_peer_host_name ) :
		m_profile(profile) ,
		m_io(nullptr) ,
		m_ssl(profile.config()) ,
		m_verified(false)
{
	mbedtls_ssl_set_bio( m_ssl.ptr() , this , doSend , doRecv , nullptr/*doRecvTimeout*/ ) ;

	// the mbedtls api uses the same function for the peer-certificate-name validation
	// and peer-host-name indication -- it also interprets wildcards in the certificate
	// cname ("www.example.com" matches "CN=*.example.com") so the peer-host-name is
	// preferred here over the peer-certificate-name
	//
	std::string name = target_peer_host_name.empty() ? required_peer_certificate_name : target_peer_host_name ;
	if( !name.empty() )
		mbedtls_ssl_set_hostname( m_ssl.ptr() , name.c_str() ) ;
}

GSsl::MbedTls::ProtocolImp::~ProtocolImp()
{
}

GSsl::Protocol::Result GSsl::MbedTls::ProtocolImp::read( char * buffer , size_t buffer_size_in , ssize_t & data_size_out )
{
	int rc = mbedtls_ssl_read( m_ssl.ptr() , reinterpret_cast<unsigned char*>(buffer) , buffer_size_in ) ;
	data_size_out = rc < 0 ? 0 : rc ;
	if( rc == 0 ) return Protocol::Result_error ; // disconnected
	size_t available = rc > 0 ? mbedtls_ssl_get_bytes_avail( m_ssl.ptr() ) : 0U ;
	return convert( "mbedtls_ssl_read" , rc , available > 0U ) ;
}

GSsl::Protocol::Result GSsl::MbedTls::ProtocolImp::write( const char * buffer_in , size_t data_size_in ,
	ssize_t & data_size_out )
{
	const unsigned char * p = reinterpret_cast<const unsigned char *>(buffer_in) ;
	ssize_t n = static_cast<ssize_t>(data_size_in) ; G_ASSERT( n >= 0 ) ;
	for(;;)
	{
		int rc = mbedtls_ssl_write( m_ssl.ptr() , p , n ) ;
		if( rc >= n )
		{
			data_size_out = static_cast<ssize_t>(data_size_in) ;
			return Protocol::Result_ok ;
		}
		else if( rc >= 0 )
		{
			n -= rc ;
			p += rc ;
		}
		else
		{
			return convert( "mbedtls_ssl_write" , rc ) ;
		}
	}
}

GSsl::Protocol::Result GSsl::MbedTls::ProtocolImp::stop()
{
	return Protocol::Result_ok ;
}

int GSsl::MbedTls::ProtocolImp::doRecvTimeout( void * This , unsigned char * p , size_t n , uint32_t timeout_ms )
{
	// with event-driven i/o the timeout is probably not useful since
	// higher layers will time out eventually
	return doRecv( This , p , n ) ;
}

int GSsl::MbedTls::ProtocolImp::doRecv( void * This , unsigned char * p , size_t n )
{
	G::ReadWrite * io = reinterpret_cast<ProtocolImp*>(This)->m_io ;
	G_ASSERT( io != nullptr ) ;
	ssize_t rc = io->read( reinterpret_cast<char*>(p) , n ) ;
	if( rc < 0 && io->eWouldBlock() ) return MBEDTLS_ERR_SSL_WANT_READ ;
	if( rc < 0 ) return MBEDTLS_ERR_NET_RECV_FAILED ; // or CONN_RESET for EPIPE or ECONNRESET
	return rc ;
}

int GSsl::MbedTls::ProtocolImp::doSend( void * This , const unsigned char * p , size_t n )
{
	G::ReadWrite * io = reinterpret_cast<ProtocolImp*>(This)->m_io ;
	G_ASSERT( io != nullptr ) ;
	ssize_t rc = io->write( reinterpret_cast<const char*>(p) , n ) ;
	if( rc < 0 && io->eWouldBlock() ) return MBEDTLS_ERR_SSL_WANT_WRITE ;
	if( rc < 0 ) return MBEDTLS_ERR_NET_SEND_FAILED ; // or CONN_RESET for EPIPE or ECONNRESET
	return rc ;
}

GSsl::Protocol::Result GSsl::MbedTls::ProtocolImp::convert( const char * fnname , int rc , bool more )
{
	// Result_ok , Result_read , Result_write , Result_error , Result_more
	if( rc == MBEDTLS_ERR_SSL_WANT_READ ) return Protocol::Result_read ;
	if( rc == MBEDTLS_ERR_SSL_WANT_WRITE ) return Protocol::Result_write ;
	if( rc == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY ) return Protocol::Result_error ;
	if( rc < 0 ) // throw on error -- moot
	{
		throw Error( fnname , rc , verifyResultString(rc) ) ;
	}
	return more ? Protocol::Result_more : Protocol::Result_ok ;
}

std::string GSsl::MbedTls::ProtocolImp::verifyResultString( int rc )
{
	if( rc == MBEDTLS_ERR_X509_CERT_VERIFY_FAILED )
	{
		int verify_result = mbedtls_ssl_get_verify_result( m_ssl.ptr() ) ; // MBEDTLS_X509_BADCERT_...
		std::vector<char> buffer( 1024U ) ;
		mbedtls_x509_crt_verify_info( &buffer[0] , buffer.size() , "" , verify_result ) ;
		buffer[buffer.size()-1U] = '\0' ;
		return G::Str::printable( &buffer[0] ) ;
	}
	else
	{
		return std::string() ;
	}
}

GSsl::Protocol::Result GSsl::MbedTls::ProtocolImp::connect( G::ReadWrite & io )
{
	m_io = &io ;
	return handshake() ;
}

GSsl::Protocol::Result GSsl::MbedTls::ProtocolImp::accept( G::ReadWrite & io )
{
	m_io = &io ;
	return handshake() ;
}

GSsl::Protocol::Result GSsl::MbedTls::ProtocolImp::handshake()
{
	int rc = mbedtls_ssl_handshake( m_ssl.ptr() ) ;
	Result result = convert( "mbedtls_ssl_handshake" , rc ) ;
	if( result == Protocol::Result_ok )
	{
		const char * vstr = "" ;
		if( m_ssl.ptr()->conf->authmode == MBEDTLS_SSL_VERIFY_NONE )
		{
			m_verified = false ;
			vstr = "peer certificate not verified" ;
		}
		else if( m_ssl.ptr()->conf->authmode == MBEDTLS_SSL_VERIFY_OPTIONAL )
		{
			int v = mbedtls_ssl_get_verify_result( m_ssl.ptr() ) ;
			m_verified = v == 0 ;
			vstr = v == 0 ? "peer certificate verified" : "peer certificate failed to verify" ;
			if( v & MBEDTLS_X509_BADCERT_EXPIRED ) vstr = "peer certificate has expired" ;
			if( v & MBEDTLS_X509_BADCERT_REVOKED ) vstr = "peer certificate has been revoked" ;
			if( v & MBEDTLS_X509_BADCERT_NOT_TRUSTED ) vstr = "peer certificate not signed by a trusted ca" ;
			if( v & MBEDTLS_X509_BADCERT_MISSING ) vstr = "peer certificate missing" ;
			if( v & MBEDTLS_X509_BADCERT_SKIP_VERIFY ) vstr = "peer certificate verification was skipped" ;
		}
		else // MBEDTLS_SSL_VERIFY_REQUIRED
		{
			m_verified = true ;
			vstr = "peer certificate verified" ;
		}

		m_peer_certificate = getPeerCertificate() ;
		m_peer_certificate_chain = m_peer_certificate ; // not implemted

		m_profile.logAt( 2 , std::string() + "protocol=" + protocol() + ", cipher=" + cipher() + ", verification=[" + vstr + "]" ) ;
	}
	return result ;
}

std::string GSsl::MbedTls::ProtocolImp::protocol() const
{
	const char * p = mbedtls_ssl_get_version( m_ssl.ptr() ) ;
	return G::Str::printable(p?std::string(p):std::string()) ;
}

std::string GSsl::MbedTls::ProtocolImp::cipher() const
{
	const char * p = mbedtls_ssl_get_ciphersuite( m_ssl.ptr() ) ;
	return G::Str::printable(p?std::string(p):std::string()) ;
}

const GSsl::Profile & GSsl::MbedTls::ProtocolImp::profile() const
{
	return m_profile ;
}

std::string GSsl::MbedTls::ProtocolImp::getPeerCertificate()
{
	std::string result ;
	const mbedtls_x509_crt * certificate = mbedtls_ssl_get_peer_cert( m_ssl.ptr() ) ;
	if( certificate != nullptr )
	{
		const char * head = "-----BEGIN CERTIFICATE-----\n" ;
		const char * tail = "-----END CERTIFICATE-----\n" ;

		size_t n = 0U ;
		unsigned char c = '\0' ;
		int rc = mbedtls_pem_write_buffer( head , tail , certificate->raw.p , certificate->raw.len , &c , 0 , &n ) ;
		if( n == 0U || rc != MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL )
			throw Error( "certificate error" ) ;
		n = n + n ; // old polarssl bug required this

		std::vector<unsigned char> buffer( n ) ;
		rc = mbedtls_pem_write_buffer( head , tail , certificate->raw.p , certificate->raw.len ,
				&buffer[0] , buffer.size() , &n ) ;
		if( n == 0 || rc != 0 )
			throw Error( "certificate error" ) ;

		result = std::string( reinterpret_cast<const char *>(&buffer[0]) , n-1U ) ;
		if( std::string(result.c_str()) != result || result.find(tail) == std::string::npos )
			throw Error( "certificate error" ) ;
	}
	return result ;
}

std::string GSsl::MbedTls::ProtocolImp::peerCertificate() const
{
	return m_peer_certificate ;
}

std::string GSsl::MbedTls::ProtocolImp::peerCertificateChain() const
{
	return m_peer_certificate_chain ;
}

bool GSsl::MbedTls::ProtocolImp::verified() const
{
	return m_verified ;
}

// ==

GSsl::MbedTls::Context::Context( const mbedtls_ssl_config * config_p )
{
	static mbedtls_ssl_context x_zero ;
	x = x_zero ;
	mbedtls_ssl_init( &x ) ;

	int rc = mbedtls_ssl_setup( &x , config_p ) ;
	if( rc ) throw Error( "mbedtls_ssl_setup" , rc ) ;
}

GSsl::MbedTls::Context::~Context()
{
	mbedtls_ssl_free( &x ) ;
}

mbedtls_ssl_context * GSsl::MbedTls::Context::ptr()
{
	return &x ;
}

mbedtls_ssl_context * GSsl::MbedTls::Context::ptr() const
{
	return const_cast<mbedtls_ssl_context*>(&x) ;
}

// ==

GSsl::MbedTls::Rng::Rng()
{
	static mbedtls_entropy_context entropy_zero ;
	entropy = entropy_zero ;
	mbedtls_entropy_init( &entropy ) ;

	static mbedtls_ctr_drbg_context x_zero ;
	x = x_zero ;
	mbedtls_ctr_drbg_init( &x ) ;

	unsigned char extra[] = "sdflkjsdlkjsdfkljxmvnxcvmxmncvx" ;
	int rc = mbedtls_ctr_drbg_seed( &x , mbedtls_entropy_func , &entropy , extra , sizeof(extra) ) ;
	if( rc != 0 )
	{
		mbedtls_entropy_free( &entropy ) ;
		throw Error( "mbedtls_ctr_drbg_init" , rc ) ;
	}
}

GSsl::MbedTls::Rng::~Rng()
{
	mbedtls_ctr_drbg_free( &x ) ;
	mbedtls_entropy_free( &entropy ) ;
}

mbedtls_ctr_drbg_context * GSsl::MbedTls::Rng::ptr()
{
	return &x ;
}

mbedtls_ctr_drbg_context * GSsl::MbedTls::Rng::ptr() const
{
	return const_cast<mbedtls_ctr_drbg_context*>(&x) ;
}

// ==

static void scrub( unsigned char *p_in , size_t n )
{
	// see also SecureZeroMemory(), memset_s(3) and mbedtls_zeroize()
	volatile unsigned char *p = p_in ;
	while( n-- )
		*p++ = 0U ;
}

GSsl::MbedTls::SecureFile::SecureFile( const std::string & path , bool with_nul )
{
	FILE * fp = nullptr ;
	try
	{
		std::filebuf f ;
		std::filebuf * fp = nullptr ;
		const char * path_p = path.c_str() ;
		{
			G::Root claim_root ;
			fp = f.open( path_p , std::ios_base::in | std::ios_base::binary ) ;
		}
		std::streamoff n = 0 ;
		bool ok = fp != nullptr ;
		if( ok ) n = fp->pubseekoff( 0 , std::ios_base::end , std::ios_base::in ) ;
		if( ok ) m_buffer.resize( static_cast<size_t>(n) ) ;
		if( ok ) fp->pubseekpos( 0 , std::ios_base::in ) ;
		if( ok ) ok = fp->sgetn( &m_buffer[0] , m_buffer.size() ) == static_cast<std::streamsize>(m_buffer.size()) ;
		if( !ok ) scrub( pu() , size() ) ;
		if( fp ) fp->close() ;
		if( ok && with_nul ) m_buffer.push_back( '\0' ) ;
	}
	catch(...)
	{
		scrub( pu() , size() ) ;
		if( fp ) std::fclose( fp ) ;
		throw ;
	}
}

GSsl::MbedTls::SecureFile::~SecureFile()
{
	scrub( pu() , size() ) ;
}

const char * GSsl::MbedTls::SecureFile::p() const
{
	static char c = '\0' ;
	return m_buffer.size() ? &m_buffer[0] : &c ;
}

const unsigned char * GSsl::MbedTls::SecureFile::pu() const
{
	return reinterpret_cast<const unsigned char*>(p()) ;
}

unsigned char * GSsl::MbedTls::SecureFile::pu()
{
	return const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(p())) ;
}

size_t GSsl::MbedTls::SecureFile::size() const
{
	return m_buffer.size() ;
}

bool GSsl::MbedTls::SecureFile::empty() const
{
	return size() == 0U ;
}

// ==

GSsl::MbedTls::Key::Key()
{
	mbedtls_pk_init( &x ) ;
}

GSsl::MbedTls::Key::~Key()
{
	mbedtls_pk_free( &x ) ;
}

void GSsl::MbedTls::Key::load( const std::string & pem_file )
{
	SecureFile file( pem_file , true ) ;
	if( file.empty() )
		throw Error( "cannot load private key from " + pem_file ) ;

	int rc = mbedtls_pk_parse_key( &x , file.pu() , file.size() , nullptr , 0 ) ;
	if( rc < 0 ) // negative error code
		throw Error( "mbedtls_pk_parse_key" , rc ) ;
	else if( rc > 0 ) // positive number of failed parts
		{;} // no-op because the file can contain non-private-key parts
}

mbedtls_pk_context * GSsl::MbedTls::Key::ptr()
{
	return &x ;
}

mbedtls_pk_context * GSsl::MbedTls::Key::ptr() const
{
	return const_cast<mbedtls_pk_context*>(&x) ;
}

// ==

GSsl::MbedTls::Certificate::Certificate() :
	m_loaded(false)
{
	mbedtls_x509_crt_init( &x ) ;
}

GSsl::MbedTls::Certificate::~Certificate()
{
	mbedtls_x509_crt_free( &x ) ;
}

void GSsl::MbedTls::Certificate::load( const std::string & path )
{
	SecureFile file( path , true ) ;
	if( file.empty() )
		throw Error( "cannot load certificates from " + path ) ;

	int rc = mbedtls_x509_crt_parse( &x , file.pu() , file.size() ) ;
	if( rc < 0 ) // negative error code
		throw Error( "mbedtls_x509_crt_parse" , rc ) ;
	else if( rc > 0 ) // positive number of failed parts
		throw Error( "mbedtls_x509_crt_parse" ) ;

	m_loaded = true ;
}

bool GSsl::MbedTls::Certificate::loaded() const
{
	return m_loaded ;
}

mbedtls_x509_crt * GSsl::MbedTls::Certificate::ptr()
{
	return loaded() ? &x : nullptr ;
}

mbedtls_x509_crt * GSsl::MbedTls::Certificate::ptr() const
{
	if( loaded() )
		return const_cast<mbedtls_x509_crt*>(&x) ;
	else
		return nullptr ;
}

// ==

GSsl::MbedTls::Error::Error( const std::string & s ) :
	m_what("tls error: "+s)
{
}

GSsl::MbedTls::Error::Error( const std::string & fnname , int rc , const std::string & more ) :
	m_what("tls error: "+fnname+"()")
{
	char buffer[200] = { '\0' } ;
	mbedtls_strerror( rc , buffer , sizeof(buffer) ) ;
	buffer[sizeof(buffer)-1] = '\0' ;

	std::ostringstream ss ;
	ss << ": mbedtls [" << G::Str::printable(buffer) << "]" ;
	if( !more.empty() )
		ss << " [" << more << "]" ;

	m_what.append( ss.str() ) ;
}

GSsl::MbedTls::Error::~Error() throw ()
{
}

const char * GSsl::MbedTls::Error::what() const throw ()
{
	return m_what.c_str() ;
}

/// \file gssl_mbedtls.cpp
