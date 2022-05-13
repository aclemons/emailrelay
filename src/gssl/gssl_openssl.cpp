//
// Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gssl_openssl.cpp
///

#include "gdef.h"
#include "gssl.h"
#include "gssl_openssl.h"
#include "ghashstate.h"
#include "gformat.h"
#include "ggettext.h"
#include "gtest.h"
#include "gstr.h"
#include "gpath.h"
#include "gprocess.h"
#include "gfile.h"
#include "groot.h"
#include "gexception.h"
#include "glog.h"
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <exception>
#include <functional>
#include <vector>
#include <sstream>
#include <utility>
#include <algorithm>
#include <memory>

GSsl::OpenSSL::LibraryImp::LibraryImp( G::StringArray & library_config , Library::LogFn log_fn , bool verbose ) :
	m_log_fn(log_fn) ,
	m_verbose(verbose) ,
	m_config(library_config)
{
#if OPENSSL_VERSION_NUMBER < 0x10100000L
	// library initialisation -- not required for OpenSSL v1.1 (August 2016)
	SSL_load_error_strings() ;
	SSL_library_init() ;
	OpenSSL_add_all_digests() ;
#endif

	// "on systems without /dev/*random devices providing entropy from the kernel the EGD entropy
	// gathering daemon can be used to collect entropy... OpenSSL automatically queries EGD when
	// entropy is ... checked via RAND_status() for the first time" (man RAND_egd(3))
	GDEF_IGNORE_RETURN RAND_status() ;

	// allocate a slot for a pointer from SSL to ProtocolImp
	m_index = SSL_get_ex_new_index( 0 , nullptr , nullptr , nullptr , nullptr ) ;
	if( m_index < 0 )
	{
		cleanup() ;
		throw Error( "SSL_get_ex_new_index" ) ;
	}
}

GSsl::OpenSSL::LibraryImp::~LibraryImp()
{
	cleanup() ;
}

void GSsl::OpenSSL::LibraryImp::cleanup()
{
#if OPENSSL_VERSION_NUMBER < 0x10100000L
	ERR_free_strings() ;
	RAND_cleanup() ;
	CRYPTO_cleanup_all_ex_data();
#endif
}

std::string GSsl::OpenSSL::LibraryImp::sid()
{
#if OPENSSL_VERSION_NUMBER < 0x10100000L
	std::string v = SSLeay_version(SSLEAY_VERSION) ;
#else
	std::string v = OpenSSL_version(OPENSSL_VERSION) ;
#endif
	return G::Str::unique( G::Str::printable(v) , ' ' ) ;
}

std::string GSsl::OpenSSL::LibraryImp::id() const
{
	return sid() ;
}

bool GSsl::OpenSSL::LibraryImp::generateKeyAvailable() const
{
	return false ;
}

std::string GSsl::OpenSSL::LibraryImp::generateKey( const std::string & ) const
{
	return std::string() ;
}

GSsl::OpenSSL::Config GSsl::OpenSSL::LibraryImp::config() const
{
	return m_config ;
}

std::string GSsl::OpenSSL::LibraryImp::credit( const std::string & prefix , const std::string & eol , const std::string & eot )
{
	std::ostringstream ss ;
	ss
		<< prefix << "This product includes software developed by the OpenSSL Project" << eol
		<< prefix << "for use in the OpenSSL Toolkit (http://www.openssl.org/)" << eol
		<< eot ;
	return ss.str() ;
}

void GSsl::OpenSSL::LibraryImp::addProfile( const std::string & profile_name , bool is_server_profile ,
	const std::string & key_file , const std::string & cert_file , const std::string & ca_file ,
	const std::string & default_peer_certificate_name , const std::string & default_peer_host_name ,
	const std::string & profile_config )
{
	std::shared_ptr<ProfileImp> profile_ptr =
		std::make_shared<ProfileImp>(*this,is_server_profile,key_file,cert_file,ca_file,
			default_peer_certificate_name,default_peer_host_name,profile_config) ;
	m_profile_map.insert( Map::value_type(profile_name,profile_ptr) ) ;
}

bool GSsl::OpenSSL::LibraryImp::hasProfile( const std::string & profile_name ) const
{
	auto p = m_profile_map.find( profile_name ) ;
	return p != m_profile_map.end() ;
}

const GSsl::Profile & GSsl::OpenSSL::LibraryImp::profile( const std::string & profile_name ) const
{
	auto p = m_profile_map.find( profile_name ) ;
	if( p == m_profile_map.end() ) throw Error( "no such profile: [" + profile_name + "]" ) ;
	return *(*p).second ;
}

GSsl::Library::LogFn GSsl::OpenSSL::LibraryImp::log() const
{
	return m_log_fn ;
}

bool GSsl::OpenSSL::LibraryImp::verbose() const
{
	return m_verbose ;
}

int GSsl::OpenSSL::LibraryImp::index() const
{
	return m_index ;
}

G::StringArray GSsl::OpenSSL::LibraryImp::digesters( bool need_state ) const
{
	G::StringArray result ;
	if( !need_state )
		result.push_back( "SHA512" ) ;
	result.push_back( "SHA256" ) ;
	result.push_back( "SHA1" ) ;
	result.push_back( "MD5" ) ;
	return result ;
}

GSsl::Digester GSsl::OpenSSL::LibraryImp::digester( const std::string & hash_type , const std::string & state , bool need_state ) const
{
	return Digester( std::make_unique<GSsl::OpenSSL::DigesterImp>(hash_type,state,need_state) ) ;
}

GSsl::OpenSSL::DigesterImp::DigesterImp( const std::string & hash_type , const std::string & state , bool need_state ) :
	m_hash_type(Type::Other) ,
	m_evp_ctx(nullptr)
{
	bool have_state = !state.empty() ;
	if( hash_type == "MD5" && ( have_state || need_state ) )
	{
		m_hash_type = Type::Md5 ;
		MD5_Init( &m_md5 ) ;
		if( have_state )
			G::HashState<16,MD5_LONG,MD5_LONG>::decode( state , m_md5.Nh , m_md5.Nl , &m_md5.A , &m_md5.B , &m_md5.C , &m_md5.D ) ;
		m_block_size = 64U ;
		m_value_size = 16U ;
		m_state_size = m_value_size + 4U ;
	}
	else if( hash_type == "SHA1" && ( have_state || need_state ) )
	{
		m_hash_type = Type::Sha1 ;
		SHA1_Init( &m_sha1 ) ;
		if( have_state )
			G::HashState<20,SHA_LONG,SHA_LONG>::decode( state , m_sha1.Nh , m_sha1.Nl , &m_sha1.h0 , &m_sha1.h1 , &m_sha1.h2 , &m_sha1.h3 , &m_sha1.h4 ) ;
		m_block_size = 64U ;
		m_value_size = 20U ;
		m_state_size = m_value_size + 4U ;
	}
	else if( hash_type == "SHA256" && ( have_state || need_state ) )
	{
		m_hash_type = Type::Sha256 ;
		SHA256_Init( &m_sha256 ) ;
		if( have_state )
			G::HashState<32,SHA_LONG,unsigned int>::decode( state , m_sha256.Nh , m_sha256.Nl , m_sha256.h ) ;
		m_block_size = 64U ;
		m_value_size = 32U ;
		m_state_size = m_value_size + 4U ;
	}
	else
	{
		m_hash_type = Type::Other ;
		m_evp_ctx = EVP_MD_CTX_create() ;

		const EVP_MD * md = EVP_get_digestbyname( hash_type.c_str() ) ;
		if( md == nullptr )
			throw Error( "unsupported hash function name: [" + hash_type + "]" ) ;

		m_block_size = static_cast<std::size_t>( EVP_MD_block_size(md) ) ;
		m_value_size = static_cast<std::size_t>( EVP_MD_size(md) ) ;
		m_state_size = 0U ; // intermediate state not available

		if( m_state_size == 0U && !state.empty() )
			throw Error( "hash state resoration not implemented for " + hash_type ) ;

		EVP_DigestInit_ex( m_evp_ctx , md , nullptr ) ;
	}
}

GSsl::OpenSSL::DigesterImp::~DigesterImp()
{
	if( m_hash_type == Type::Other )
		EVP_MD_CTX_destroy( m_evp_ctx ) ;
}

std::size_t GSsl::OpenSSL::DigesterImp::blocksize() const
{
	return m_block_size ;
}

std::size_t GSsl::OpenSSL::DigesterImp::valuesize() const
{
	return m_value_size ;
}

std::size_t GSsl::OpenSSL::DigesterImp::statesize() const
{
	return m_state_size ;
}

std::string GSsl::OpenSSL::DigesterImp::state()
{
	if( m_hash_type == Type::Md5 )
		return G::HashState<16,MD5_LONG,MD5_LONG>::encode( m_md5.Nh , m_md5.Nl , m_md5.A , m_md5.B , m_md5.C , m_md5.D ) ;
	else if( m_hash_type == Type::Sha1 )
		return G::HashState<20,SHA_LONG,SHA_LONG>::encode( m_sha1.Nh , m_sha1.Nl , m_sha1.h0 , m_sha1.h1 , m_sha1.h2 , m_sha1.h3 , m_sha1.h4 ) ;
	else if( m_hash_type == Type::Sha256 )
		return G::HashState<32,SHA_LONG,SHA_LONG>::encode( m_sha256.Nh , m_sha256.Nl , m_sha256.h ) ;
	else
		return std::string() ; // not available
}

void GSsl::OpenSSL::DigesterImp::add( const std::string & data )
{
	if( m_hash_type == Type::Md5 )
		MD5_Update( &m_md5 , data.data() , data.size() ) ;
	else if( m_hash_type == Type::Sha1 )
		SHA1_Update( &m_sha1 , data.data() , data.size() ) ;
	else if( m_hash_type == Type::Sha256 )
		SHA256_Update( &m_sha256 , data.data() , data.size() ) ;
	else
		EVP_DigestUpdate( m_evp_ctx , data.data() , data.size() ) ;
}

std::string GSsl::OpenSSL::DigesterImp::value()
{
	std::vector<unsigned char> output ;
	std::size_t n = 0U ;
	if( m_hash_type == Type::Md5 )
	{
		n = MD5_DIGEST_LENGTH ;
		output.resize( n ) ;
		MD5_Final( &output[0] , &m_md5 ) ;
	}
	else if( m_hash_type == Type::Sha1 )
	{
		n = SHA_DIGEST_LENGTH ;
		output.resize( n ) ;
		SHA1_Final( &output[0] , &m_sha1 ) ;
	}
	else if( m_hash_type == Type::Sha256 )
	{
		n = SHA256_DIGEST_LENGTH ;
		output.resize( n ) ;
		SHA256_Final( &output[0] , &m_sha256 ) ;
	}
	else
	{
		unsigned int output_size = 0 ;
		output.resize( EVP_MAX_MD_SIZE ) ;
		EVP_DigestFinal_ex( m_evp_ctx , &output[0] , &output_size ) ;
		n = static_cast<std::size_t>(output_size) ;
	}
	G_ASSERT( n == valuesize() ) ;
	const char * p = reinterpret_cast<char*>(&output[0]) ;
	return std::string(p,n) ;
}

// ==

GSsl::OpenSSL::ProfileImp::ProfileImp( const LibraryImp & library_imp , bool is_server_profile ,
	const std::string & key_file , const std::string & cert_file , const std::string & ca_path ,
	const std::string & default_peer_certificate_name , const std::string & default_peer_host_name ,
	const std::string & profile_config ) :
		m_library_imp(library_imp) ,
		m_default_peer_certificate_name(default_peer_certificate_name) ,
		m_default_peer_host_name(default_peer_host_name) ,
		m_ssl_ctx(nullptr,std::function<void(SSL_CTX*)>(deleter))
{
	using G::format ;
	using G::txt ;
	Config extra_config = m_library_imp.config() ;
	if( !profile_config.empty() )
	{
		G::StringArray profile_config_list = G::Str::splitIntoTokens( profile_config , "," ) ;
		extra_config = Config( profile_config_list ) ;
		if( !profile_config_list.empty() )
			G_WARNING( "GSsl::OpenSSL::ProfileImp::ctor: tls-config: tls " << (is_server_profile?"server":"client")
				<< " profile configuration ignored: [" << G::Str::join(",",profile_config_list) << "]" ) ;
	}

	if( m_ssl_ctx == nullptr )
	{
		Config::Fn version_fn = extra_config.fn( is_server_profile ) ;
		m_ssl_ctx.reset( SSL_CTX_new( version_fn() ) ) ;
		if( m_ssl_ctx != nullptr )
			apply( extra_config ) ;
	}

	if( m_ssl_ctx == nullptr )
		throw Error( "SSL_CTX_new" , ERR_get_error() ) ;

	if( !key_file.empty() )
	{
		G::Root claim_root ;
		if( !G::File::exists(key_file) )
			G_WARNING( "GSsl::Profile: " << format(txt("cannot open ssl key file: %1%")) % key_file ) ;

		check( SSL_CTX_use_PrivateKey_file(m_ssl_ctx.get(),key_file.c_str(),SSL_FILETYPE_PEM) ,
			"use_PrivateKey_file" , key_file ) ;
	}

	if( !cert_file.empty() )
	{
		G::Root claim_root ;
		if( !G::File::exists(cert_file) )
			G_WARNING( "GSsl::Profile: " << format(txt("cannot open ssl certificate file: %1%")) % cert_file ) ;

		check( SSL_CTX_use_certificate_chain_file(m_ssl_ctx.get(),cert_file.c_str()) ,
			"use_certificate_chain_file" , cert_file ) ;
	}

	if( ca_path.empty() )
	{
		// ask for peer certificates but just log them without verifying - we don't
		// use set_client_CA_list() so we allow the client to not send a certificate
		SSL_CTX_set_verify( m_ssl_ctx.get() , SSL_VERIFY_PEER , verifyPass ) ;
	}
	else if( ca_path == "<none>" )
	{
		// dont ask for client certificates (server-side)
		SSL_CTX_set_verify( m_ssl_ctx.get() , SSL_VERIFY_NONE , nullptr ) ;
	}
	else if( ca_path == "<default>" )
	{
		// ask for certificates, make sure they verify against the default ca database, and check the name in the certificate (if given)
		bool no_verify = extra_config.noverify() ;
		SSL_CTX_set_verify( m_ssl_ctx.get() , no_verify ? SSL_VERIFY_NONE : (SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT) , verifyPeerName ) ;
		check( SSL_CTX_set_default_verify_paths( m_ssl_ctx.get() ) , "set_default_verify_paths" ) ;
	}
	else
	{
		// ask for certificates, make sure they verify against the given ca database, and check the name in the certificate (if given)
		bool ca_path_is_dir = G::File::isDirectory( ca_path , std::nothrow ) ;
		const char * ca_file_p = ca_path_is_dir ? nullptr : ca_path.c_str() ;
		const char * ca_dir_p = ca_path_is_dir ? ca_path.c_str() : nullptr ;
		bool no_verify = extra_config.noverify() ;
		SSL_CTX_set_verify( m_ssl_ctx.get() , no_verify ? SSL_VERIFY_NONE : (SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT) , verifyPeerName ) ;
		check( SSL_CTX_load_verify_locations( m_ssl_ctx.get() , ca_file_p , ca_dir_p ) , "load_verify_locations" , ca_path ) ;
	}

	SSL_CTX_set_cipher_list( m_ssl_ctx.get() , "DEFAULT" ) ;
	SSL_CTX_set_session_cache_mode( m_ssl_ctx.get() , SSL_SESS_CACHE_OFF ) ;
	if( is_server_profile )
	{
		static std::string x = "GSsl.OpenSSL." + G::Path(G::Process::exe()).basename() ;
		SSL_CTX_set_session_id_context( m_ssl_ctx.get() , reinterpret_cast<const unsigned char *>(x.data()) , x.size() ) ;
	}
}

GSsl::OpenSSL::ProfileImp::~ProfileImp()
= default;

void GSsl::OpenSSL::ProfileImp::deleter( SSL_CTX * p )
{
	if( p != nullptr )
		SSL_CTX_free( p ) ;
}

std::unique_ptr<GSsl::ProtocolImpBase> GSsl::OpenSSL::ProfileImp::newProtocol( const std::string & peer_certificate_name ,
	const std::string & peer_host_name ) const
{
	return std::make_unique<OpenSSL::ProtocolImp>( *this ,
			peer_certificate_name.empty()?defaultPeerCertificateName():peer_certificate_name ,
			peer_host_name.empty()?defaultPeerHostName():peer_host_name ) ; // up-cast
}

SSL_CTX * GSsl::OpenSSL::ProfileImp::p() const
{
	return const_cast<SSL_CTX*>( m_ssl_ctx.get() ) ;
}

const GSsl::OpenSSL::LibraryImp & GSsl::OpenSSL::ProfileImp::lib() const
{
	return m_library_imp ;
}

const std::string & GSsl::OpenSSL::ProfileImp::defaultPeerCertificateName() const
{
	return m_default_peer_certificate_name ;
}

const std::string & GSsl::OpenSSL::ProfileImp::defaultPeerHostName() const
{
	return m_default_peer_host_name ;
}

void GSsl::OpenSSL::ProfileImp::check( int rc , const std::string & fnname_tail , const std::string & file )
{
	if( rc != 1 )
	{
		std::string fnname = "SSL_CTX_" + fnname_tail ;
		throw Error( fnname , ERR_get_error() , file ) ;
	}
}

void GSsl::OpenSSL::ProfileImp::apply( const Config & config )
{
	#if GCONFIG_HAVE_OPENSSL_MIN_MAX
	if( config.min_() )
		SSL_CTX_set_min_proto_version( m_ssl_ctx.get() , config.min_() ) ;

	if( config.max_() )
		SSL_CTX_set_max_proto_version( m_ssl_ctx.get() , config.max_() ) ;
	#endif

	if( config.reset() != 0L )
		SSL_CTX_clear_options( m_ssl_ctx.get() , config.reset() ) ;

	if( config.set() != 0L )
		SSL_CTX_set_options( m_ssl_ctx.get() , config.set() ) ;
}

int GSsl::OpenSSL::ProfileImp::verifyPass( int /*ok*/ , X509_STORE_CTX * )
{
	return 1 ;
}

int GSsl::OpenSSL::ProfileImp::verifyPeerName( int ok , X509_STORE_CTX * ctx )
{
	try
	{
		if( ok && X509_STORE_CTX_get_error_depth(ctx) == 0 )
		{
			SSL * ssl = static_cast<SSL*>( X509_STORE_CTX_get_ex_data( ctx , SSL_get_ex_data_X509_STORE_CTX_idx() ) ) ;
			if( ssl == nullptr )
				return 0 ; // never gets here

			OpenSSL::LibraryImp & library = dynamic_cast<OpenSSL::LibraryImp&>( Library::impstance() ) ;
			OpenSSL::ProtocolImp * protocol = static_cast<OpenSSL::ProtocolImp*>( SSL_get_ex_data(ssl,library.index()) ) ;
			if( protocol == nullptr )
				return 0 ; // never gets here

			std::string required_peer_certificate_name = protocol->requiredPeerCertificateName() ;
			if( !required_peer_certificate_name.empty() )
			{
				X509 * cert = X509_STORE_CTX_get_current_cert( ctx ) ;
				std::string subject = name(X509_get_subject_name(cert)) ;
				G::StringArray subject_parts = G::Str::splitIntoTokens( subject , "/" ) ;
				bool found = std::find( subject_parts.begin() , subject_parts.end() , "CN="+required_peer_certificate_name ) != subject_parts.end() ;
				library.log()( 2 , "certificate-subject=[" + subject + "] required-peer-name=[" + required_peer_certificate_name + "] ok=" + (found?"1":"0") ) ;
				if( !found )
				{
					ok = 0 ;
				}
			}
		}
		return ok ;
	}
	catch(...) // callback from c code
	{
		return 0 ;
	}
}

std::string GSsl::OpenSSL::ProfileImp::name( X509_NAME * x509_name )
{
	if( x509_name == nullptr ) return std::string() ;
	std::vector<char> buffer( 2048U ) ; // 200 in openssl code
	X509_NAME_oneline( x509_name , &buffer[0] , buffer.size() ) ;
	buffer.back() = '\0' ;
	return G::Str::printable( std::string(&buffer[0]) ) ;
}

// ==

GSsl::OpenSSL::ProtocolImp::ProtocolImp( const ProfileImp & profile , const std::string & required_peer_certificate_name ,
	const std::string & target_peer_host_name ) :
		m_ssl(nullptr,std::function<void(SSL*)>(deleter)) ,
		m_log_fn(profile.lib().log()) ,
		m_verbose(profile.lib().verbose()) ,
		m_fd_set(false) ,
		m_required_peer_certificate_name(required_peer_certificate_name) ,
		m_verified(false)
{
	m_ssl.reset( SSL_new(profile.p()) ) ;
	if( m_ssl == nullptr )
		throw Error( "SSL_new" , ERR_get_error() ) ;

	// TODO feature test for SSL_set_tlsext_host_name() ?
	if( !target_peer_host_name.empty() )
		SSL_set_tlsext_host_name( m_ssl.get() , target_peer_host_name.c_str() ) ;

	// store a pointer from SSL to ProtocolImp
	SSL_set_ex_data( m_ssl.get() , profile.lib().index() , this ) ;
}

GSsl::OpenSSL::ProtocolImp::~ProtocolImp()
= default;

void GSsl::OpenSSL::ProtocolImp::deleter( SSL * p )
{
	if( p != nullptr )
		SSL_free( p ) ;
}

void GSsl::OpenSSL::ProtocolImp::clearErrors()
{
	// "The current thread's error queue must be empty before [SSL_connect,SSL_accept,SSL_read,SSL_write] "
	// "is attempted, or SSL_get_error() will not work reliably."
	Error::clearErrors() ;
}

int GSsl::OpenSSL::ProtocolImp::error( const char * op , int rc ) const
{
	int e = SSL_get_error( m_ssl.get() , rc ) ;
	logErrors( op , rc , e , Protocol::str(convert(e)) ) ;
	return e ;
}

GSsl::Protocol::Result GSsl::OpenSSL::ProtocolImp::convert( int e )
{
	if( e == SSL_ERROR_WANT_READ ) return Protocol::Result::read ;
	if( e == SSL_ERROR_WANT_WRITE ) return Protocol::Result::write ;
	return Protocol::Result::error ;
}

GSsl::Protocol::Result GSsl::OpenSSL::ProtocolImp::connect( G::ReadWrite & io )
{
	set( io.fd() ) ;
	return connect() ;
}

GSsl::Protocol::Result GSsl::OpenSSL::ProtocolImp::accept( G::ReadWrite & io )
{
	set( io.fd() ) ;
	return accept() ;
}

void GSsl::OpenSSL::ProtocolImp::set( int fd )
{
	if( !m_fd_set )
	{
		int rc = SSL_set_fd( m_ssl.get() , fd ) ;
		if( rc == 0 )
			throw Error( "SSL_set_fd" , ERR_get_error() ) ;

		if( G::Test::enabled("log-openssl-bio") ) // log bio activity directly to stderr
		{
			BIO_set_callback( SSL_get_rbio(m_ssl.get()) , BIO_debug_callback ) ;
			BIO_set_callback( SSL_get_wbio(m_ssl.get()) , BIO_debug_callback ) ;
		}

		m_fd_set = true ;
	}
}

GSsl::Protocol::Result GSsl::OpenSSL::ProtocolImp::connect()
{
	clearErrors() ;
	int rc = SSL_connect( m_ssl.get() ) ;
	if( rc >= 1 )
	{
		saveResult() ;
		return Protocol::Result::ok ;
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

GSsl::Protocol::Result GSsl::OpenSSL::ProtocolImp::accept()
{
	clearErrors() ;
	int rc = SSL_accept( m_ssl.get() ) ;
	if( rc >= 1 )
	{
		saveResult() ;
		return Protocol::Result::ok ;
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

void GSsl::OpenSSL::ProtocolImp::saveResult()
{
	m_peer_certificate = Certificate(SSL_get_peer_certificate(m_ssl.get()),true).str() ;
	m_peer_certificate_chain = CertificateChain(SSL_get_peer_cert_chain(m_ssl.get())).str() ;
	m_verified = !m_peer_certificate.empty() && SSL_get_verify_result(m_ssl.get()) == X509_V_OK ;
}

GSsl::Protocol::Result GSsl::OpenSSL::ProtocolImp::shutdown()
{
	int rc = SSL_shutdown( m_ssl.get() ) ;
	if( rc == 0 || rc == 1 )
		return Protocol::Result::ok ;
	else
		return convert( rc ) ;
}

GSsl::Protocol::Result GSsl::OpenSSL::ProtocolImp::read( char * buffer , std::size_t buffer_size_in , ssize_t & read_size )
{
	read_size = 0 ;

	clearErrors() ;
	int buffer_size = static_cast<int>(buffer_size_in) ;
	int rc = SSL_read( m_ssl.get() , buffer , buffer_size ) ;
	if( rc > 0 )
	{
		read_size = static_cast<ssize_t>(rc) ;
		return SSL_pending(m_ssl.get()) ? Protocol::Result::more : Protocol::Result::ok ;
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

GSsl::Protocol::Result GSsl::OpenSSL::ProtocolImp::write( const char * buffer , std::size_t size_in , ssize_t & size_out )
{
	size_out = 0 ;
	clearErrors() ;
	int size = static_cast<int>(size_in) ;
	int rc = SSL_write( m_ssl.get() , buffer , size ) ;
	if( rc > 0 )
	{
		size_out = static_cast<ssize_t>(rc) ;
		return Protocol::Result::ok ;
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

std::string GSsl::OpenSSL::ProtocolImp::peerCertificate() const
{
	return m_peer_certificate ;
}

std::string GSsl::OpenSSL::ProtocolImp::peerCertificateChain() const
{
	return m_peer_certificate_chain ;
}

std::string GSsl::OpenSSL::ProtocolImp::cipher() const
{
	const SSL_CIPHER * cipher = SSL_get_current_cipher( const_cast<SSL*>(m_ssl.get()) ) ;
	const char * name = cipher ? SSL_CIPHER_get_name(cipher) : nullptr ;
	return name ? G::Str::printable(std::string(name)) : std::string() ;
}

std::string GSsl::OpenSSL::ProtocolImp::protocol() const
{
	#if OPENSSL_VERSION_NUMBER < 0x10100000L
		int v = SSL_version( m_ssl.get() ) ;
	#else
		const SSL_SESSION * session = SSL_get_session( const_cast<SSL*>(m_ssl.get()) ) ;
		if( session == nullptr ) return std::string() ;
		int v = SSL_SESSION_get_protocol_version( session ) ;
	#endif
	#ifdef TLS1_VERSION
		if( v == TLS1_VERSION ) return "TLSv1.0" ; // cf. mbedtls
	#endif
	#ifdef TLS1_1_VERSION
		if( v == TLS1_1_VERSION ) return "TLSv1.1" ;
	#endif
	#ifdef TLS1_2_VERSION
		if( v == TLS1_2_VERSION ) return "TLSv1.2" ;
	#endif
	#ifdef TLS1_3_VERSION
		if( v == TLS1_3_VERSION ) return "TLSv1.3" ;
	#else
		if( v == 0x304 ) return "TLSv1.3" ; // grr libressl
	#endif
	return "#" + G::Str::fromInt(v) ;
}

bool GSsl::OpenSSL::ProtocolImp::verified() const
{
	return m_verified ;
}

void GSsl::OpenSSL::ProtocolImp::logErrors( const std::string & op , int rc , int e , const std::string & strerr ) const
{
	if( m_log_fn != nullptr )
	{
		if( m_verbose )
		{
			std::ostringstream ss ;
			ss << op << ": rc=" << rc << ": error " << e << " => " << strerr ;
			(*m_log_fn)( 1 , ss.str() ) ; // 1 => verbose-debug
		}

		for( int i = 2 ; i < 10000 ; i++ )
		{
			unsigned long ee = ERR_get_error() ;
			if( ee == 0 ) break ;
			Error eee( op , ee ) ;
			(*m_log_fn)( 3 , std::string() + eee.what() ) ; // 3 => errors-and-warnings
		}
	}
}

std::string GSsl::OpenSSL::ProtocolImp::requiredPeerCertificateName() const
{
	return m_required_peer_certificate_name ;
}

// ==

GSsl::OpenSSL::Error::Error( const std::string & s ) :
	std::runtime_error( "tls error: " + s )
{
}

GSsl::OpenSSL::Error::Error( const std::string & fnname , unsigned long e ) :
	std::runtime_error( "tls error: " + fnname + "(): [" + text(e) + "]" )
{
	clearErrors() ;
}

GSsl::OpenSSL::Error::Error( const std::string & fnname , unsigned long e , const std::string & file ) :
	std::runtime_error( "tls error: " + fnname + "(): [" + text(e) + "]: file=[" + file + "]" )
{
	clearErrors() ;
}

void GSsl::OpenSSL::Error::clearErrors()
{
	for( int i = 0 ; ERR_get_error() && i < 10000 ; i++ )
		{;}
}

std::string GSsl::OpenSSL::Error::text( unsigned long e )
{
	std::vector<char> v( 300 ) ;
	ERR_error_string_n( e , &v[0] , v.size() ) ;
	std::string s( &v[0] , v.size() ) ;
	return std::string( s.c_str() ) ; // NOLINT copy up to first null character
}

// ==

GSsl::OpenSSL::CertificateChain::CertificateChain( STACK_OF(X509) * chain )
{
	for( int i = 0 ; chain != nullptr && i < sk_X509_num(chain) ; i++ )
	{
		void * p = sk_X509_value(chain,i) ; if( p == nullptr ) break ;
		X509 * x509 = static_cast<X509*>(p) ;
		m_str.append( Certificate(x509,false).str() ) ;
	}
}

std::string GSsl::OpenSSL::CertificateChain::str() const
{
	return m_str ;
}

// ==

GSsl::OpenSSL::Certificate::Certificate( X509 * x509 , bool do_free )
{
	if( x509 == nullptr ) return ;
	BIO * bio = BIO_new( BIO_s_mem() ) ;
	int rc = PEM_write_bio_X509( bio , x509 ) ;
	if( !rc ) return ;
	BUF_MEM * mem = nullptr ;
	BIO_get_mem_ptr( bio , &mem ) ;
	std::size_t n = mem ? static_cast<std::size_t>(mem->length) : 0U ;
	const char * p = mem ? mem->data : nullptr ;
	std::string data = p&&n ? std::string(p,n) : std::string() ;
	BIO_free( bio ) ;
	if( do_free ) X509_free( x509 ) ;

	// sanitise to be strictly printable with embedded newlines
	std::string result = G::Str::printable( data , '\0' ) ;
	G::Str::replaceAll( result , std::string(1U,'\0')+"n" , "\n" ) ;
	G::Str::replaceAll( result , std::string(1U,'\0') , "\\" ) ;
	m_str = result ;
}

std::string GSsl::OpenSSL::Certificate::str() const
{
	return m_str ;
}

// ==

GSsl::OpenSSL::Config::Config( G::StringArray & cfg ) :
	m_min(0) ,
	m_max(0) ,
	m_options_set(0L) ,
	m_options_reset(0L) ,
	m_noverify(consume(cfg,"noverify"))
{
	#if GCONFIG_HAVE_OPENSSL_TLS_METHOD
		m_server_fn = TLS_server_method ;
		m_client_fn = TLS_client_method ;
	#else
		m_server_fn = SSLv23_server_method ;
		m_client_fn = SSLv23_client_method ;
	#endif

	#if GCONFIG_HAVE_OPENSSL_MIN_MAX

		#ifdef SSL3_VERSION
			if( consume(cfg,"sslv3") ) m_min = SSL3_VERSION ;
			if( consume(cfg,"-sslv3") ) m_max = SSL3_VERSION ;
		#endif

		#ifdef TLS1_VERSION
			if( consume(cfg,"tlsv1.0") ) m_min = TLS1_VERSION ;
			if( consume(cfg,"-tlsv1.0") ) m_max = TLS1_VERSION ;
		#endif

		#ifdef TLS1_1_VERSION
			if( consume(cfg,"tlsv1.1") ) m_min = TLS1_1_VERSION ;
			if( consume(cfg,"-tlsv1.1") ) m_max = TLS1_1_VERSION ;
		#endif

		#ifdef TLS1_2_VERSION
			if( consume(cfg,"tlsv1.2") ) m_min = TLS1_2_VERSION ;
			if( consume(cfg,"-tlsv1.2") ) m_max = TLS1_2_VERSION ;
		#endif

		#ifdef TLS1_3_VERSION
			if( consume(cfg,"tlsv1.3") ) m_min = TLS1_3_VERSION ;
			if( consume(cfg,"-tlsv1.3") ) m_max = TLS1_3_VERSION ;
		#endif

	#endif

	#ifdef SSL_OP_ALL
		if( consume(cfg,"op_all") ) m_options_set |= SSL_OP_ALL ;
	#endif

	#ifdef SSL_OP_NO_TICKET
		if( consume(cfg,"op_no_ticket") ) m_options_set |= SSL_OP_NO_TICKET ;
	#endif

	#ifdef SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION
		if( consume(cfg,"op_no_resumption") ) m_options_set |= SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION ;
	#endif

	#ifdef SSL_OP_CIPHER_SERVER_PREFERENCE
		if( consume(cfg,"op_server_preference") ) m_options_set |= SSL_OP_CIPHER_SERVER_PREFERENCE ;
	#endif
}

bool GSsl::OpenSSL::Config::consume( G::StringArray & list , const std::string & item )
{
	return LibraryImp::consume( list , item ) ;
}

GSsl::OpenSSL::Config::Fn GSsl::OpenSSL::Config::fn( bool server )
{
	return server ? m_server_fn : m_client_fn ;
}

long GSsl::OpenSSL::Config::set() const
{
	return m_options_set ;
}

long GSsl::OpenSSL::Config::reset() const
{
	return m_options_reset ;
}

int GSsl::OpenSSL::Config::min_() const
{
	return m_min ;
}

int GSsl::OpenSSL::Config::max_() const
{
	return m_max ;
}

bool GSsl::OpenSSL::Config::noverify() const
{
	return m_noverify ;
}
