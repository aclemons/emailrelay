//
// Copyright (C) 2001-2021 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gssl_mbedtls.cpp
///

#include "gdef.h"
#include "gssl.h"
#include "gssl_mbedtls.h"
#include "ghashstate.h"
#include "gpath.h"
#include "gsleep.h"
#include "groot.h"
#include "gtest.h"
#include "gstr.h"
#include "gfile.h"
#include "gprocess.h"
#include "gscope.h"
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
#include <vector>
#include <exception>
#include <iomanip>

// macro magic to provide function-name/function-pointer arguments for call()
#ifdef FN
#undef FN
#endif
#define FN( fn ) (#fn),(fn)

// in newer versions of the mbedtls library hashing functions like mbed_whatever_ret()
// returning an integer are preferred, compared to mbed_whatever() returning void
#define GSSL_PASTE_IMP( a , b ) a##b
#define GSSL_PASTE( a , b ) GSSL_PASTE_IMP( a , b )
#if MBEDTLS_VERSION_NUMBER >= 0x02070000
#define FN_RET( fn ) (#fn),(GSSL_PASTE(fn,_ret))
#else
#define FN_RET( fn ) (#fn),(fn)
#endif

namespace GSsl
{
	namespace MbedTls
	{
		void randomFillImp( char * p , std::size_t n ) ;
		int randomFill( void * , unsigned char * output , std::size_t len , std::size_t * olen ) ;

		template <typename F, typename... Args>
		typename std::enable_if< !std::is_same<void,typename std::result_of<F(Args...)>::type>::value >::type
		call( const char * fname , F fn , Args&&... args )
		{
			int rc = fn( std::forward<Args>(args)... ) ;
			if( rc )
				throw Error( fname , rc ) ;
		}

		template <typename F, typename... Args>
		typename std::enable_if< std::is_same<void,typename std::result_of<F(Args...)>::type>::value >::type
		call( const char * , F fn , Args&&... args )
		{
			fn( std::forward<Args>(args)... ) ;
		}

		template <typename T>
		struct ScopeExitCleanup /// Calls a mbedtls free function on descruction.
		{
			explicit ScopeExitCleanup( void (*fn)(T*) ) : m_fn(fn) , m_p(nullptr) {}
			ScopeExitCleanup( void (*fn)(T*) , T * p ) : m_fn(fn) , m_p(p) {}
			~ScopeExitCleanup() { reset() ; }
			void reset( T * p = nullptr ) { if(m_p) m_fn(m_p) ; m_p = p ; }
			void release() { m_p = nullptr ; }
			void (*m_fn)(T*) ;
			T * m_p ;
		} ;

		template <typename T>
		struct X /// Initialises and frees an mbedtls object on construction and destruction.
		{
			X( void (*init)(T*) , void (*free)(T*) ) : m_free(free) { init(&x) ; }
			~X() { m_free(&x) ; }
			T * ptr() { return &x ; }
			const T * ptr() const { return &x ; }
			T x ;
			void (*m_free)(T*) ;
			X * operator&() = delete ;
			const X * operator&() const = delete ;
			X( const X<T> & ) = delete ;
			X( X<T> && ) = delete ;
			void operator=( const X<T> & ) = delete ;
			void operator=( X<T> && ) = delete ;
		} ;
	}
}

GSsl::MbedTls::LibraryImp::LibraryImp( G::StringArray & library_config , Library::LogFn log_fn , bool verbose ) :
	m_log_fn(log_fn) ,
	m_config(library_config)
{
	mbedtls_debug_set_threshold( verbose ? 3 : 1 ) ; // "Messages that have a level over the threshold value are ignored."
}

GSsl::MbedTls::LibraryImp::~LibraryImp()
= default;

const GSsl::MbedTls::Rng & GSsl::MbedTls::LibraryImp::rng() const
{
	return m_rng ;
}

void GSsl::MbedTls::LibraryImp::addProfile( const std::string & profile_name , bool is_server_profile ,
	const std::string & key_file , const std::string & cert_file , const std::string & ca_file ,
	const std::string & default_peer_certificate_name , const std::string & default_peer_host_name ,
	const std::string & profile_config )
{
	std::shared_ptr<ProfileImp> profile_ptr =
		std::make_shared<ProfileImp>(*this,is_server_profile,key_file,cert_file,ca_file,
			default_peer_certificate_name,default_peer_host_name,profile_config) ;
	m_profile_map.insert( Map::value_type(profile_name,profile_ptr) ) ;
}

bool GSsl::MbedTls::LibraryImp::hasProfile( const std::string & profile_name ) const
{
	auto p = m_profile_map.find( profile_name ) ;
	return p != m_profile_map.end() ;
}

const GSsl::Profile & GSsl::MbedTls::LibraryImp::profile( const std::string & profile_name ) const
{
	auto p = m_profile_map.find( profile_name ) ;
	if( p == m_profile_map.end() ) throw Error( "no such profile: [" + profile_name + "]" ) ;
	return *(*p).second.get() ;
}

GSsl::Library::LogFn GSsl::MbedTls::LibraryImp::log() const
{
	return m_log_fn ;
}

std::string GSsl::MbedTls::LibraryImp::version()
{
	std::vector<char> buffer( 100U ) ; // "at least 9"
	G_ASSERT( buffer.size() >= 9U ) ;
	mbedtls_version_get_string( &buffer[0] ) ;
	buffer[buffer.size()-1U] = '\0' ;
	return G::Str::printable( std::string(&buffer[0]) ) ;
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

bool GSsl::MbedTls::LibraryImp::generateKeyAvailable() const
{
	return true ;
}

std::string GSsl::MbedTls::LibraryImp::generateKey( const std::string & issuer_name ) const
{
	// see mbedtls/programs/pkey/gen_key.c ...

	X<mbedtls_entropy_context> entropy( mbedtls_entropy_init , mbedtls_entropy_free ) ;
	if( !G::is_windows() )
	{
		const int threshold = 32 ;
		call( FN(mbedtls_entropy_add_source) , entropy.ptr() , randomFill , nullptr ,
			threshold , MBEDTLS_ENTROPY_SOURCE_STRONG ) ;
	}

	X<mbedtls_ctr_drbg_context> drbg( mbedtls_ctr_drbg_init , mbedtls_ctr_drbg_free ) ;
	{
		std::string seed_name = "gssl_mbedtls" ;
		auto seed_name_p = reinterpret_cast<const unsigned char*>( seed_name.data() ) ;
		call( FN(mbedtls_ctr_drbg_seed) , drbg.ptr() , mbedtls_entropy_func , entropy.ptr() , seed_name_p , seed_name.size() ) ;
	}

	X<mbedtls_pk_context> key( mbedtls_pk_init , mbedtls_pk_free ) ;
	{
		const mbedtls_pk_type_t type = MBEDTLS_PK_RSA;
		const unsigned int keysize = 4096U ;
		const int exponent = 65537 ;
		call( FN(mbedtls_pk_setup) , key.ptr() , mbedtls_pk_info_from_type(type) ) ;
		call( FN(mbedtls_rsa_gen_key) , mbedtls_pk_rsa(key.x) , mbedtls_ctr_drbg_random , drbg.ptr() , keysize , exponent ) ;
	}

	std::string s_key ;
	{
		std::vector<unsigned char> pk_buffer( 16000U ) ;
		call( FN(mbedtls_pk_write_key_pem) , key.ptr() , &pk_buffer[0] , pk_buffer.size() ) ;
		pk_buffer[pk_buffer.size()-1] = 0 ;
		s_key = reinterpret_cast<const char*>( &pk_buffer[0] ) ;
	}

	// see mbedtls/programs/x509/cert_write.c ...

	X<mbedtls_mpi> mpi( mbedtls_mpi_init , mbedtls_mpi_free ) ;
	{
		const char * serial = "1" ;
		call( FN(mbedtls_mpi_read_string) , mpi.ptr() , 10 , serial ) ;
	}

	X<mbedtls_x509write_cert> crt( mbedtls_x509write_crt_init , mbedtls_x509write_crt_free ) ;
	{
		const char * not_before = "20200101000000" ;
		const char * not_after = "20401231235959" ;
		const int is_ca = 0 ;
		const int max_pathlen = -1 ;
		call( FN(mbedtls_x509write_crt_set_subject_key) , crt.ptr() , key.ptr() ) ;
		call( FN(mbedtls_x509write_crt_set_issuer_key) , crt.ptr() , key.ptr() ) ;
		call( FN(mbedtls_x509write_crt_set_subject_name) , crt.ptr() , issuer_name.c_str()/*sic*/ ) ;
		call( FN(mbedtls_x509write_crt_set_issuer_name) , crt.ptr() , issuer_name.c_str() ) ;
		call( FN(mbedtls_x509write_crt_set_version) , crt.ptr() , MBEDTLS_X509_CRT_VERSION_3 ) ;
		call( FN(mbedtls_x509write_crt_set_md_alg) , crt.ptr() , MBEDTLS_MD_SHA256 ) ;
		call( FN(mbedtls_x509write_crt_set_serial) , crt.ptr() , mpi.ptr() ) ;
		call( FN(mbedtls_x509write_crt_set_validity) , crt.ptr() , not_before , not_after ) ;
		call( FN(mbedtls_x509write_crt_set_basic_constraints) , crt.ptr() , is_ca , max_pathlen ) ;
		call( FN(mbedtls_x509write_crt_set_subject_key_identifier) , crt.ptr() ) ;
		call( FN(mbedtls_x509write_crt_set_authority_key_identifier) , crt.ptr() ) ;
	}

	std::string s_crt ;
	{
		std::vector<unsigned char> crt_buffer( 4096 ) ;
		call( FN(mbedtls_x509write_crt_pem) , crt.ptr() , &crt_buffer[0] , crt_buffer.size() , mbedtls_ctr_drbg_random , drbg.ptr() ) ;
		crt_buffer[crt_buffer.size()-1] = 0 ;
		s_crt = reinterpret_cast<const char*>( &crt_buffer[0] ) ;
	}

	return s_key + s_crt ;
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
	return { "MD5" , "SHA1" , "SHA256" } ;
}

GSsl::Digester GSsl::MbedTls::LibraryImp::digester( const std::string & hash_type , const std::string & state , bool need_state ) const
{
	return Digester( std::make_unique<GSsl::MbedTls::DigesterImp>(hash_type,state,need_state) ) ;
}

// ==

GSsl::MbedTls::Config::Config( G::StringArray & config ) :
	m_noverify(consume(config,"noverify")),
	m_clientnoverify(consume(config,"clientnoverify")),
	m_servernoverify(consume(config,"servernoverify")),
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

bool GSsl::MbedTls::Config::clientnoverify() const
{
	return m_clientnoverify ;
}

bool GSsl::MbedTls::Config::servernoverify() const
{
	return m_servernoverify ;
}

bool GSsl::MbedTls::Config::consume( G::StringArray & list , const std::string & item )
{
	return LibraryImp::consume( list , item ) ;
}

// ==

GSsl::MbedTls::DigesterImp::DigesterImp( const std::string & hash_name , const std::string & state , bool )
{
	if( hash_name == "MD5" )
	{
		m_hash_type = Type::Md5 ;
		m_block_size = 64U ;
		m_value_size = 16U ;
		m_state_size = m_value_size + 4U ;

		mbedtls_md5_init( &m_md5 ) ;
		if( state.empty() )
			call( FN_RET(mbedtls_md5_starts) , &m_md5 ) ;
		else
			G::HashState<16,uint32_t,uint32_t>::decode( state , m_md5.state , m_md5.total[0] ) ;
	}
	else if( hash_name == "SHA1" )
	{
		m_hash_type = Type::Sha1 ;
		m_block_size = 64U ;
		m_value_size = 20U ;
		m_state_size = m_value_size + 4U ;

		mbedtls_sha1_init( &m_sha1 ) ;
		if( state.empty() )
			call( FN_RET(mbedtls_sha1_starts) , &m_sha1 ) ;
		else
			G::HashState<20,uint32_t,uint32_t>::decode( state , m_sha1.state , m_sha1.total[0] ) ;
	}
	else if( hash_name == "SHA256" )
	{
		m_hash_type = Type::Sha256 ;
		m_block_size = 64U ;
		m_value_size = 32U ;
		m_state_size = m_value_size + 4U ;

		mbedtls_sha256_init( &m_sha256 ) ;
		if( state.empty() )
			call( FN_RET(mbedtls_sha256_starts) , &m_sha256 , 0 ) ;
		else
			G::HashState<32,uint32_t,uint32_t>::decode( state , m_sha256.state , m_sha256.total[0] ) ;
	}
	else
	{
		throw Error( "invalid hash function" ) ;
	}
}

GSsl::MbedTls::DigesterImp::~DigesterImp()
{
	if( m_hash_type == Type::Md5 )
		mbedtls_md5_free( &m_md5 ) ;
	else if( m_hash_type == Type::Sha1 )
		mbedtls_sha1_free( &m_sha1 ) ;
	else if( m_hash_type == Type::Sha256 )
		mbedtls_sha256_free( &m_sha256 ) ;
}

void GSsl::MbedTls::DigesterImp::add( const std::string & s )
{
	if( m_hash_type == Type::Md5 )
		call( FN_RET(mbedtls_md5_update) , &m_md5 , reinterpret_cast<const unsigned char*>(s.data()) , s.size() ) ;
	else if( m_hash_type == Type::Sha1 )
		call( FN_RET(mbedtls_sha1_update) , &m_sha1 , reinterpret_cast<const unsigned char*>(s.data()) , s.size() ) ;
	else if( m_hash_type == Type::Sha256 )
		call( FN_RET(mbedtls_sha256_update) , &m_sha256 , reinterpret_cast<const unsigned char*>(s.data()) , s.size() ) ;
}

std::string GSsl::MbedTls::DigesterImp::value()
{
	if( m_hash_type == Type::Md5 )
	{
		unsigned char buffer[16] ;
		call( FN_RET(mbedtls_md5_finish) , &m_md5 , buffer ) ;
		return std::string( reinterpret_cast<const char*>(buffer) , sizeof(buffer) ) ;
	}
	else if( m_hash_type == Type::Sha1 )
	{
		unsigned char buffer[20] ;
		call( FN_RET(mbedtls_sha1_finish) , &m_sha1 , buffer ) ;
		return std::string( reinterpret_cast<const char*>(buffer) , sizeof(buffer) ) ;
	}
	else if( m_hash_type == Type::Sha256 )
	{
		unsigned char buffer[32] ;
		call( FN_RET(mbedtls_sha256_finish) , &m_sha256 , buffer ) ;
		return std::string( reinterpret_cast<const char*>(buffer) , sizeof(buffer) ) ;
	}
	else
	{
		return std::string() ;
	}
}

std::string GSsl::MbedTls::DigesterImp::state()
{
	if( m_hash_type == Type::Md5 )
		return G::HashState<16,uint32_t,uint32_t>::encode( m_md5.state , m_md5.total[0] ) ;
	else if( m_hash_type == Type::Sha1 )
		return G::HashState<20,uint32_t,uint32_t>::encode( m_sha1.state , m_sha1.total[0] ) ;
	else if( m_hash_type == Type::Sha256 )
		return G::HashState<32,uint32_t,uint32_t>::encode( m_sha256.state , m_sha256.total[0] ) ;
	else
		return std::string() ;
}

std::size_t GSsl::MbedTls::DigesterImp::blocksize() const
{
	return m_block_size ;
}

std::size_t GSsl::MbedTls::DigesterImp::valuesize() const
{
	return m_value_size ;
}

std::size_t GSsl::MbedTls::DigesterImp::statesize() const
{
	return m_state_size ;
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
	ScopeExitCleanup<mbedtls_ssl_config> cleanup( mbedtls_ssl_config_free ) ;

	// use library config, or override with profile config
	Config extra_config = library_imp.config() ;
	if( !profile_config.empty() )
	{
		G::StringArray profile_config_list = G::Str::splitIntoTokens( profile_config , "," ) ;
		extra_config = Config( profile_config_list ) ;
		if( !profile_config_list.empty() ) // ie. residue not consumed by Config ctor
			G_WARNING( "GSsl::MbedTls::ProfileImp::ctor: tls-config: tls " << (is_server_profile?"server":"client")
				<< " profile configuration ignored: [" << G::Str::join(",",profile_config_list) << "]" ) ;
	}

	// initialise the mbedtls_ssl_config structure
	{
		m_config = mbedtls_ssl_config{} ;
		mbedtls_ssl_config_init( &m_config ) ;
		cleanup.reset( &m_config ) ;
		int rc = mbedtls_ssl_config_defaults( &m_config ,
			is_server_profile ? MBEDTLS_SSL_IS_SERVER : MBEDTLS_SSL_IS_CLIENT , // see also mbedtls_ssl_conf_endpoint()
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
		int authmode = 0 ;
		if( ca_path.empty() )
		{
			// verify if possible, but continue on failure - see mbedtls_ssl_get_verify_result()
			authmode = MBEDTLS_SSL_VERIFY_OPTIONAL ;

			// mbedtls 2.4.2 can incorrectly fail to handshake when OPTIONAL so provide some more tweakability
			if( is_server_profile && extra_config.servernoverify() )
				authmode = MBEDTLS_SSL_VERIFY_NONE ;
			else if( !is_server_profile && extra_config.clientnoverify() )
				authmode = MBEDTLS_SSL_VERIFY_NONE ;
			else if( mbedtls_version_get_number() <= 0x02040200 )
				G_WARNING_ONCE( "GSsl::MbedTls::LibraryImp::ctor: mbedtls library version " << LibraryImp::version() << " is deprecated" ) ;
		}
		else if( ca_path == "<none>" )
		{
			// dont verify
			authmode = MBEDTLS_SSL_VERIFY_NONE ;
		}
		else if( ca_path == "<default>" )
		{
			// verify against the default ca database
			std::string ca_path_default = "/etc/ssl/certs/ca-certificates.crt" ; // see debian "man update-ca-certificates"
			m_ca_list.load( ca_path_default ) ;
			bool no_verify = extra_config.noverify() ;
			mbedtls_ssl_conf_ca_chain( &m_config , m_ca_list.ptr() , crl() ) ;
			authmode = no_verify ? MBEDTLS_SSL_VERIFY_OPTIONAL : MBEDTLS_SSL_VERIFY_REQUIRED ;
		}
		else
		{
			// verify against the given ca database
			m_ca_list.load( ca_path ) ;
			bool no_verify = extra_config.noverify() ;
			mbedtls_ssl_conf_ca_chain( &m_config , m_ca_list.ptr() , crl() ) ;
			authmode = no_verify ? MBEDTLS_SSL_VERIFY_OPTIONAL : MBEDTLS_SSL_VERIFY_REQUIRED ;
		}
		mbedtls_ssl_conf_authmode( &m_config , authmode ) ;
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

std::unique_ptr<GSsl::ProtocolImpBase> GSsl::MbedTls::ProfileImp::newProtocol( const std::string & peer_certificate_name ,
	const std::string & peer_host_name ) const
{
	return std::make_unique<MbedTls::ProtocolImp>( *this ,
		peer_certificate_name.empty()?defaultPeerCertificateName():peer_certificate_name ,
		peer_host_name.empty()?defaultPeerHostName():peer_host_name ) ; // upcast
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
		static_cast<ProfileImp*>(This)->doDebug( level_in , file , line , message ) ;
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
= default;

GSsl::Protocol::Result GSsl::MbedTls::ProtocolImp::read( char * buffer , std::size_t buffer_size_in , ssize_t & data_size_out )
{
	int rc = mbedtls_ssl_read( m_ssl.ptr() , reinterpret_cast<unsigned char*>(buffer) , buffer_size_in ) ;
	data_size_out = rc < 0 ? 0 : rc ;
	if( rc == 0 ) return Protocol::Result::error ; // disconnected
	std::size_t available = rc > 0 ? mbedtls_ssl_get_bytes_avail( m_ssl.ptr() ) : 0U ;
	return convert( "mbedtls_ssl_read" , rc , available > 0U ) ;
}

GSsl::Protocol::Result GSsl::MbedTls::ProtocolImp::write( const char * buffer_in , std::size_t data_size_in ,
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
			return Protocol::Result::ok ;
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

GSsl::Protocol::Result GSsl::MbedTls::ProtocolImp::shutdown()
{
	int rc = mbedtls_ssl_close_notify( m_ssl.ptr() ) ;
	return convert( "mbedtls_ssl_close_notify" , rc ) ;
}

int GSsl::MbedTls::ProtocolImp::doRecvTimeout( void * This , unsigned char * p , std::size_t n , uint32_t /*timeout_ms*/ )
{
	// with event-driven i/o the timeout is probably not useful since
	// higher layers will time out eventually
	return doRecv( This , p , n ) ;
}

int GSsl::MbedTls::ProtocolImp::doRecv( void * This , unsigned char * p , std::size_t n )
{
	G::ReadWrite * io = static_cast<ProtocolImp*>(This)->m_io ;
	G_ASSERT( io != nullptr ) ;
	ssize_t rc = io->read( reinterpret_cast<char*>(p) , n ) ;
	if( rc < 0 && io->eWouldBlock() ) return MBEDTLS_ERR_SSL_WANT_READ ;
	if( rc < 0 ) return MBEDTLS_ERR_NET_RECV_FAILED ; // or CONN_RESET for EPIPE or ECONNRESET
	return rc ;
}

int GSsl::MbedTls::ProtocolImp::doSend( void * This , const unsigned char * p , std::size_t n )
{
	G::ReadWrite * io = static_cast<ProtocolImp*>(This)->m_io ;
	G_ASSERT( io != nullptr ) ;
	ssize_t rc = io->write( reinterpret_cast<const char*>(p) , n ) ;
	if( rc < 0 && io->eWouldBlock() ) return MBEDTLS_ERR_SSL_WANT_WRITE ;
	if( rc < 0 ) return MBEDTLS_ERR_NET_SEND_FAILED ; // or CONN_RESET for EPIPE or ECONNRESET
	return rc ;
}

GSsl::Protocol::Result GSsl::MbedTls::ProtocolImp::convert( const char * fnname , int rc , bool more )
{
	// ok , read , write , error , more
	if( rc == MBEDTLS_ERR_SSL_WANT_READ ) return Protocol::Result::read ;
	if( rc == MBEDTLS_ERR_SSL_WANT_WRITE ) return Protocol::Result::write ;
	if( rc == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY ) return Protocol::Result::error ;
	if( rc < 0 ) // throw on error -- moot
	{
		throw Error( fnname , rc , verifyResultString(rc) ) ;
	}
	return more ? Protocol::Result::more : Protocol::Result::ok ;
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
	if( result == Protocol::Result::ok )
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
		m_peer_certificate_chain = m_peer_certificate ; // not implemented

		m_profile.logAt( 2 , std::string("certificate verification: [") + vstr + "]" ) ;
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

		std::size_t n = 0U ;
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
	x = mbedtls_ssl_context{} ;
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
	entropy = mbedtls_entropy_context{} ;
	mbedtls_entropy_init( &entropy ) ;

	x = mbedtls_ctr_drbg_context{} ;
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

static void scrub( unsigned char *p_in , std::size_t n )
{
	// see also SecureZeroMemory(), memset_s(3), explicit_bzero(BSD) and mbedtls_zeroize()
	volatile unsigned char *p = p_in ;
	while( n-- )
		*p++ = 0U ;
}

GSsl::MbedTls::SecureFile::SecureFile( const std::string & path , bool with_nul )
{
	std::filebuf f ;
	std::filebuf * fp = nullptr ;
	try
	{
		{
			G::Root claim_root ;
			fp = G::File::open( f , path , G::File::InOut::In ) ;
		}
		std::streamoff n = 0 ;
		bool ok = fp != nullptr ;
		if( ok ) n = fp->pubseekoff( 0 , std::ios_base::end , std::ios_base::in ) ;
		if( ok ) m_buffer.resize( static_cast<std::size_t>(n) ) ;
		if( ok ) fp->pubseekpos( 0 , std::ios_base::in ) ;
		if( ok ) ok = fp->sgetn( &m_buffer[0] , m_buffer.size() ) == static_cast<std::streamsize>(m_buffer.size()) ;
		if( !ok ) scrub( pu() , size() ) ;
		if( fp ) { fp->close() ; fp = nullptr ; }
		if( ok && with_nul ) m_buffer.push_back( '\0' ) ;
	}
	catch(...)
	{
		scrub( pu() , size() ) ;
		if( fp ) fp->close() ;
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
	return m_buffer.empty() ? &c : &m_buffer[0] ;
}

const unsigned char * GSsl::MbedTls::SecureFile::pu() const
{
	return reinterpret_cast<const unsigned char*>(p()) ;
}

unsigned char * GSsl::MbedTls::SecureFile::pu()
{
	return const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(p())) ;
}

std::size_t GSsl::MbedTls::SecureFile::size() const
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

GSsl::MbedTls::Certificate::Certificate()
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
	std::runtime_error("tls error: "+s)
{
}

GSsl::MbedTls::Error::Error( const std::string & fnname , int rc , const std::string & more ) :
	std::runtime_error(format(fnname,rc,more))
{
}

std::string GSsl::MbedTls::Error::format( const std::string & fnname , int rc , const std::string & more )
{
	std::vector<char> buffer( 200U ) ;
	buffer[0] = '\0' ;
	mbedtls_strerror( rc , &buffer[0] , buffer.size() ) ;
	buffer[buffer.size()-1U] = '\0' ;

	std::ostringstream ss ;
	ss << "tls error: " << fnname << "(): mbedtls [" << G::Str::printable(&buffer[0]) << "]" ;
	if( !more.empty() )
		ss << " [" << more << "]" ;

	return ss.str() ;
}

// ==

void GSsl::MbedTls::randomFillImp( char * p , std::size_t n )
{
	int fd = G::File::open( "/dev/random" , G::File::InOutAppend::In ) ; // not "/dev/urandom" here
	if( fd < 0 )
	{
		int e = G::Process::errno_() ;
		throw G::Exception( "cannot open /dev/random" , G::Process::strerror(e) ) ;
	}
	G::ScopeExit closer( [&](){G::File::close(fd);} ) ;

	while( n )
	{
		ssize_t nread_s = G::File::read( fd , p , n ) ;
		if( nread_s < 0 )
			throw Error( "cannot read /dev/random" ) ;

		std::size_t nread = static_cast<std::size_t>( nread_s ) ;
		if( nread > n )
			throw Error( "cannot read /dev/random" ) ;

		n -= nread ;
		p += nread ;
		sleep( 1 ) ;
	}
}

int GSsl::MbedTls::randomFill( void * , unsigned char * output , std::size_t len , std::size_t * olen )
{
	*olen = 0U ;
	randomFillImp( reinterpret_cast<char*>(output) , len ) ;
	*olen = len ;
	return 0 ;
}
