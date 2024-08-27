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
/// \file gssl_openssl.h
///

#ifndef G_SSL_OPENSSL_H
#define G_SSL_OPENSSL_H

#include "gdef.h"
#include "gssl.h"
#include "gstringview.h"
#include "gassert.h"
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <memory>
#include <stdexcept>
#include <functional>
#include <map>

#if OPENSSL_VERSION_NUMBER < 0x10100000L
#error "openssl is too old"
#endif
#ifndef GCONFIG_HAVE_OPENSSL_HASH_FUNCTIONS
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
#define GCONFIG_HAVE_OPENSSL_HASH_FUNCTIONS 0
#else
#define GCONFIG_HAVE_OPENSSL_HASH_FUNCTIONS 1
#endif
#endif

// debugging...
//  * network logging
//     $ sudo tcpdump -s 0 -n -i eth0 -X tcp port 587
//  * emailrelay smtp proxy to gmail
//     $ emailrelay --client-tls --forward-to smtp.gmail.com:587 ...
//  * openssl smtp client to gmail
//     $ openssl s_client -tls1 -msg -debug -starttls smtp -crlf -connect smtp.gmail.com:587
//  * certificate
//     $ openssl req -x509 -nodes -subj /CN=example.com -newkey rsa:1024 -keyout example.pem -out example.pem
//     $ cp example.pem /etc/ssl/certs/
//     $ cd /etc/ssl/certs && ln -s example.pem `openssl x509 -noout -hash -in example.pem`.0
//  * openssl server (without smtp)
//     $ openssl s_server -accept 10025 -cert /etc/ssl/certs/example.pem -debug -msg -tls1
//

namespace GSsl
{
	namespace OpenSSL /// A namespace for implementing the GSsl interface using the OpenSSL library.
	{
		class Error ;
		class Certificate ;
		class CertificateChain ;
		class LibraryImp ;
		class ProfileImp ;
		class ProtocolImp ;
		class DigesterImp ;
		class Config ;
	}
}

//| \class GSsl::OpenSSL::Certificate
/// Holds a certificate taken from an OpenSSL X509 structure.
///
class GSsl::OpenSSL::Certificate
{
public:
	Certificate( X509* , bool do_free ) ;
	std::string str() const ;

private:
	std::string m_str ;
} ;

//| \class GSsl::OpenSSL::Config
/// Holds protocol version information, etc.
///
class GSsl::OpenSSL::Config
{
public:
	using Fn = const SSL_METHOD *(*)() ;
	explicit Config( G::StringArray & config ) ;
	Fn fn( bool server ) ;
	long set() const ;
	long reset() const ;
	int min_() const ;
	int max_() const ;
	bool noverify() const ;

private:
	static bool consume( G::StringArray & , std::string_view ) ;
	static int map( int , int ) ;

private:
	Fn m_server_fn ;
	Fn m_client_fn ;
	int m_min {0} ;
	int m_max {0} ;
	long m_options_set {0L} ;
	long m_options_reset {0L} ;
	bool m_noverify ;
} ;

//| \class GSsl::OpenSSL::CertificateChain
/// Holds a certificate chain taken from a stack of OpenSSL X509 structures.
///
class GSsl::OpenSSL::CertificateChain
{
public:
	explicit CertificateChain( STACK_OF(X509) * chain ) ;
	std::string str() const ;

private:
	std::string m_str ;
} ;

//| \class GSsl::OpenSSL::Error
/// An exception class for GSsl::OpenSSL classes.
///
class GSsl::OpenSSL::Error : public std::runtime_error
{
public:
	explicit Error( const std::string & ) ;
	Error( const std::string & , unsigned long ) ;
	Error( const std::string & , unsigned long , const std::string & path ) ;
	static void clearErrors() ;

private:
	static std::string text( unsigned long ) ;
} ;

//| \class GSsl::OpenSSL::ProfileImp
/// An implementation of the GSsl::Profile interface for OpenSSL.
///
class GSsl::OpenSSL::ProfileImp : public Profile
{
public:
	using Error = OpenSSL::Error ;

	ProfileImp( const LibraryImp & , bool is_server_profile , const std::string & key_file ,
		const std::string & cert_file , const std::string & ca_file ,
		const std::string & default_peer_certificate_name , const std::string & default_peer_host_name ,
		const std::string & profile_config ) ;
	~ProfileImp() override ;
	SSL_CTX * p() const ;
	const LibraryImp & lib() const ;
	const std::string & defaultPeerCertificateName() const ;
	const std::string & defaultPeerHostName() const ;
	void apply( const Config & ) ;

private: // overrides
	std::unique_ptr<ProtocolImpBase> newProtocol( const std::string & , const std::string & ) const override ;

public:
	ProfileImp( const ProfileImp & ) = delete ;
	ProfileImp( ProfileImp && ) = delete ;
	ProfileImp & operator=( const ProfileImp & ) = delete ;
	ProfileImp & operator=( ProfileImp && ) = delete ;

private:
	static void check( int , const std::string & , const std::string & = {} ) ;
	static int verifyPass( int , X509_STORE_CTX * ) ;
	static int verifyPeerName( int , X509_STORE_CTX * ) ;
	static std::string name( X509_NAME * ) ;
	static void deleter( SSL_CTX * ) ;

private:
	const LibraryImp & m_library_imp ;
	const std::string m_default_peer_certificate_name ;
	const std::string m_default_peer_host_name ;
	std::unique_ptr<SSL_CTX,std::function<void(SSL_CTX*)>> m_ssl_ctx ;
} ;

//| \class GSsl::OpenSSL::LibraryImp
/// An implementation of the GSsl::LibraryImpBase interface for OpenSSL.
///
class GSsl::OpenSSL::LibraryImp : public LibraryImpBase
{
public:
	using Error = GSsl::OpenSSL::Error ;

	LibraryImp( G::StringArray & library_config , Library::LogFn , bool verbose ) ;
	~LibraryImp() override ;
	Config config() const ;
	bool noverify() const ;

	Library::LogFn log() const ;
	bool verbose() const ;
	int index() const ;
	static std::string credit( const std::string & prefix , const std::string & eol , const std::string & eot ) ;
	static std::string sid() ;

private: // overrides
	void addProfile( const std::string & name , bool is_server_profile ,
		const std::string & key_file , const std::string & cert_file , const std::string & ca_file ,
		const std::string & default_peer_certificate_name , const std::string & default_peer_host_name ,
		const std::string & profile_config ) override ;
	bool hasProfile( const std::string & ) const override ;
	const GSsl::Profile & profile( const std::string & ) const override ;
	std::string id() const override ;
	G::StringArray digesters( bool ) const override ;
	Digester digester( const std::string & , const std::string & , bool ) const override ;

public:
	LibraryImp( const LibraryImp & ) = delete ;
	LibraryImp( LibraryImp && ) = delete ;
	LibraryImp & operator=( const LibraryImp & ) = delete ;
	LibraryImp & operator=( LibraryImp && ) = delete ;

private:
	static void cleanup() ;

private:
	using Map = std::map<std::string,std::shared_ptr<ProfileImp>> ;
	std::string m_library_config ;
	Library::LogFn m_log_fn ;
	bool m_verbose ;
	Map m_profile_map ;
	int m_index {-1} ; // SSL_get_ex_new_index()
	Config m_config ;
} ;

//| \class GSsl::OpenSSL::ProtocolImp
/// An implementation of the GSsl::ProtocolImpBase interface for OpenSSL.
///
class GSsl::OpenSSL::ProtocolImp : public ProtocolImpBase
{
public:
	using Result = Protocol::Result ;
	using Error = OpenSSL::Error ;
	using Certificate = OpenSSL::Certificate ;
	using CertificateChain = OpenSSL::CertificateChain ;

	ProtocolImp( const ProfileImp & , const std::string & , const std::string & ) ;
	~ProtocolImp() override ;
	std::string requiredPeerCertificateName() const ;

private: // overrides
	Result connect( G::ReadWrite & ) override ;
	Result accept( G::ReadWrite & ) override ;
	Result shutdown() override ;
	Result read( char * buffer , std::size_t buffer_size , ssize_t & read_size ) override ;
	Result write( const char * buffer , std::size_t size_in , ssize_t & size_out ) override ;
	std::string peerCertificate() const override ;
	std::string peerCertificateChain() const override ;
	std::string protocol() const override ;
	std::string cipher() const override ;
	bool verified() const override ;

public:
	ProtocolImp( const ProtocolImp & ) = delete ;
	ProtocolImp( ProtocolImp && ) = delete ;
	ProtocolImp & operator=( const ProtocolImp & ) = delete ;
	ProtocolImp & operator=( ProtocolImp && ) = delete ;

private:
	int error( const char * , int ) const ;
	void set( int ) ;
	Result connect() ;
	Result accept() ;
	static Result convert( int ) ;
	static void clearErrors() ;
	void logErrors( const std::string & op , int rc , int e , const std::string & ) const ;
	void saveResult() ;
	static void deleter( SSL * ) ;

private:
	std::unique_ptr<SSL,std::function<void(SSL*)>> m_ssl ;
	Library::LogFn m_log_fn ;
	bool m_verbose ;
	bool m_fd_set {false} ;
	std::string m_required_peer_certificate_name ;
	std::string m_peer_certificate ;
	std::string m_peer_certificate_chain ;
	bool m_verified {false} ;
} ;

//| \class GSsl::OpenSSL::DigesterImp
/// An implementation of the GSsl::DigesterImpBase interface for OpenSSL.
///
class GSsl::OpenSSL::DigesterImp : public GSsl::DigesterImpBase
{
public:
	DigesterImp( const std::string & , const std::string & , bool ) ;
	~DigesterImp() override ;

private: // overrides
	void add( std::string_view ) override ;
	std::string value() override ;
	std::string state() override ;
	std::size_t blocksize() const noexcept override ;
	std::size_t valuesize() const noexcept override ;
	std::size_t statesize() const noexcept override ;

public:
	DigesterImp( const DigesterImp & ) = delete ;
	DigesterImp( DigesterImp && ) = delete ;
	DigesterImp & operator=( const DigesterImp & ) = delete ;
	DigesterImp & operator=( DigesterImp && ) = delete ;

private:
	enum class Type { Md5 , Sha1 , Sha256 , Other } ;
	Type m_hash_type {Type::Other} ;
	#if GCONFIG_HAVE_OPENSSL_HASH_FUNCTIONS
	MD5_CTX m_md5 {} ;
	SHA_CTX m_sha1 {} ;
	SHA256_CTX m_sha256 {} ;
	#endif
	EVP_MD_CTX * m_evp_ctx {nullptr} ;
	std::size_t m_block_size {0U} ;
	std::size_t m_value_size {0U} ;
	std::size_t m_state_size {0U} ;
} ;

#endif
