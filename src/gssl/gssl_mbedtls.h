//
// Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gssl_mbedtls.h
///

#ifndef G_SSL_MBEDTLS_H
#define G_SSL_MBEDTLS_H

#include "gdef.h"
#include "gssl.h"
#include "gssl_mbedtls_headers.h"
#include <memory>
#include <stdexcept>
#include <vector>
#include <map>

namespace GSsl
{
	namespace MbedTls /// A namespace for implementing the GSsl interface using the mbedtls library.
	{
		class Certificate ;
		class Rng ;
		class Key ;
		class Context ;
		class Error ;
		class SecureFile ;
		class LibraryImp ;
		class ProfileImp ;
		class ProtocolImp ;
		class DigesterImp ;
		class Config ;
	}
}

//| \class GSsl::MbedTls::Certificate
/// Holds a mbedtls_x509_crt structure.
///
class GSsl::MbedTls::Certificate
{
public:
	Certificate() ;
	~Certificate() ;
	void load( const std::string & file ) ;
	bool loaded() const ;
	mbedtls_x509_crt * ptr() ;
	mbedtls_x509_crt * ptr() const ;

public:
	Certificate( const Certificate & ) = delete ;
	Certificate( Certificate && ) = delete ;
	Certificate & operator=( const Certificate & ) = delete ;
	Certificate & operator=( Certificate && ) = delete ;

private:
	bool m_loaded{false} ;
	mbedtls_x509_crt x ;
} ;

//| \class GSsl::MbedTls::Rng
/// Holds a mbedtls_ctr_drbg_context structure.
///
class GSsl::MbedTls::Rng
{
public:
	Rng() ;
	~Rng() ;
	mbedtls_ctr_drbg_context * ptr() ;
	mbedtls_ctr_drbg_context * ptr() const ;

public:
	Rng( const Rng & ) = delete ;
	Rng( Rng && ) = delete ;
	Rng & operator=( const Rng & ) = delete ;
	Rng & operator=( Rng && ) = delete ;

private:
	mbedtls_ctr_drbg_context x ; // "counter-mode deterministic random byte generator"
	mbedtls_entropy_context entropy ;
} ;

//| \class GSsl::MbedTls::Key
/// Holds a mbedtls_pk_context structure.
///
class GSsl::MbedTls::Key
{
public:
	Key() ;
	~Key() ;
	void load( const std::string & file , const Rng & ) ;
	mbedtls_pk_context * ptr() ;
	mbedtls_pk_context * ptr() const ;

public:
	Key( const Key & ) = delete ;
	Key( Key && ) = delete ;
	Key & operator=( const Key & ) = delete ;
	Key & operator=( Key && ) = delete ;

private:
	mbedtls_pk_context x ;
} ;

//| \class GSsl::MbedTls::Context
/// Holds a mbedtls_ssl_context structure.
///
class GSsl::MbedTls::Context
{
public:
	explicit Context( const mbedtls_ssl_config * ) ;
	~Context() ;
	mbedtls_ssl_context * ptr() ;
	mbedtls_ssl_context * ptr() const ;

public:
	Context( const Context & ) = delete ;
	Context( Context && ) = delete ;
	Context & operator=( const Context & ) = delete ;
	Context & operator=( Context && ) = delete ;

private:
	mbedtls_ssl_context x ;
} ;

//| \class GSsl::MbedTls::Error
/// An exception class for GSsl::MbedTls classes.
///
class GSsl::MbedTls::Error : public std::runtime_error
{
public:
	explicit Error( const std::string & ) ;
	Error( const std::string & , int rc , const std::string & more = {} ) ;

private:
	static std::string format( const std::string & , int , const std::string & ) ;
} ;

//| \class GSsl::MbedTls::SecureFile
/// An interface for reading a sensitive file and then overwriting
/// its contents in memory.
///
class GSsl::MbedTls::SecureFile
{
public:
	SecureFile( const std::string & path , bool with_counted_nul ) ;
	~SecureFile() ;
	const char * p() const ;
	const unsigned char * pu() const ;
	unsigned char * pu() ;
	std::size_t size() const ;
	bool empty() const ;

public:
	SecureFile( const SecureFile & ) = delete ;
	SecureFile( SecureFile && ) = delete ;
	SecureFile & operator=( const SecureFile & ) = delete ;
	SecureFile & operator=( SecureFile && ) = delete ;

private:
	static std::size_t fileSize( std::filebuf & ) ;
	static bool fileRead( std::filebuf & , char * , std::size_t ) ;
	static void scrub( char * , std::size_t ) noexcept ;
	static void clear( std::vector<char> & ) noexcept ;

private:
	std::vector<char> m_buffer ;
} ;

//| \class GSsl::MbedTls::Config
/// Holds protocol version information, etc.
///
class GSsl::MbedTls::Config
{
public:
	explicit Config( G::StringArray & config ) ;
	int min_() const noexcept ;
	int max_() const noexcept ;
	bool noverify() const noexcept ;
	bool clientnoverify() const noexcept ;
	bool servernoverify() const noexcept ;

private:
	static bool consume( G::StringArray & , G::string_view ) ;

private:
	bool m_noverify ;
	bool m_clientnoverify ;
	bool m_servernoverify ;
	int m_min ;
	int m_max ;
} ;

//| \class GSsl::MbedTls::LibraryImp
/// An implementation of the GSsl::LibraryImpBase interface for mbedtls.
///
class GSsl::MbedTls::LibraryImp : public LibraryImpBase
{
public:
	using Rng = MbedTls::Rng ;

	LibraryImp( G::StringArray & , Library::LogFn , bool verbose ) ;
	~LibraryImp() override ;
	const Rng & rng() const ;
	Library::LogFn log() const ;
	Config config() const ;
	static std::string credit( const std::string & , const std::string & , const std::string & ) ;
	static std::string sid() ;
	static std::string version() ; // eg. "1.2.3"

private: // overrides
	void addProfile( const std::string & profile_name , bool is_server_profile ,
		const std::string & key_file , const std::string & cert_file , const std::string & ca_file ,
		const std::string & default_peer_certificate_name , const std::string & default_peer_host_name ,
		const std::string & profile_config ) override ;
	bool hasProfile( const std::string & profile_name ) const override ;
	const Profile & profile( const std::string & profile_name ) const override ;
	std::string id() const override ;
	G::StringArray digesters( bool ) const override ;
	Digester digester( const std::string & , const std::string & , bool ) const override ;

public:
	LibraryImp( const LibraryImp & ) = delete ;
	LibraryImp( LibraryImp && ) = delete ;
	LibraryImp & operator=( const LibraryImp & ) = delete ;
	LibraryImp & operator=( LibraryImp && ) = delete ;

private:
	static int minVersionFrom( G::StringArray & ) ;
	static int maxVersionFrom( G::StringArray & ) ;

private:
	using Map = std::map<std::string,std::shared_ptr<ProfileImp>> ;
	Library::LogFn m_log_fn ;
	Config m_config ;
	Map m_profile_map ;
	Rng m_rng ;
} ;

//| \class GSsl::MbedTls::ProfileImp
/// An implementation of the GSsl::Profile interface for mbedtls.
///
class GSsl::MbedTls::ProfileImp : public Profile
{
public:
	using Key = MbedTls::Key ;
	using Error = MbedTls::Error ;
	using Certificate = MbedTls::Certificate ;

	ProfileImp( const LibraryImp & library_imp , bool is_server , const std::string & key_file ,
		const std::string & cert_file , const std::string & ca_file ,
		const std::string & default_peer_certificate_name , const std::string & default_peer_host_name ,
		const std::string & profile_config ) ;
	~ProfileImp() override ;
	const mbedtls_ssl_config * config() const ;
	mbedtls_x509_crl * crl() const ;
	void logAt( int level_out , std::string ) const ;
	const std::string & defaultPeerCertificateName() const ;
	const std::string & defaultPeerHostName() const ;
	int authmode() const ;

private: // overrides
	std::unique_ptr<ProtocolImpBase> newProtocol( const std::string & , const std::string & ) const override ;

public:
	ProfileImp( const ProfileImp & ) = delete ;
	ProfileImp( ProfileImp && ) = delete ;
	ProfileImp & operator=( const ProfileImp & ) = delete ;
	ProfileImp & operator=( ProfileImp && ) = delete ;

private:
	static void onDebug( void * , int , const char * , int , const char * ) ;
	void doDebug( int , const char * , int , const char * ) ;

private:
	const LibraryImp & m_library_imp ;
	const std::string m_default_peer_certificate_name ;
	const std::string m_default_peer_host_name ;
	mbedtls_ssl_config m_config ;
	Key m_pk ;
	Certificate m_certificate ;
	Certificate m_ca_list ;
	int m_authmode ;
} ;

//| \class GSsl::MbedTls::ProtocolImp
/// An implementation of the GSsl::ProtocolImpBase interface for mbedtls.
///
class GSsl::MbedTls::ProtocolImp : public ProtocolImpBase
{
public:
	using Result = Protocol::Result ;
	using Context = MbedTls::Context ;
	using Error = MbedTls::Error ;

	ProtocolImp( const ProfileImp & , const std::string & , const std::string & ) ;
	~ProtocolImp() override ;

	static int doSend( void * , const unsigned char * , std::size_t ) ;
	static int doRecv( void * , unsigned char * , std::size_t ) ;
	static int doRecvTimeout( void * , unsigned char * , std::size_t , uint32_t ) ;
	const Profile & profile() const ;

private: // overrides
	Result connect( G::ReadWrite & ) override ;
	Result accept( G::ReadWrite & ) override ;
	Result read( char * buffer , std::size_t buffer_size_in , ssize_t & data_size_out ) override ;
	Result write( const char * buffer , std::size_t data_size_in , ssize_t & data_size_out ) override ;
	Result shutdown() override ;
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
	Result convert( const char * , int , bool more = false ) ;
	Result handshake() ;
	std::string getPeerCertificate() ;
	std::string verifyResultString( int ) ;

private:
	const ProfileImp & m_profile ;
	G::ReadWrite * m_io ;
	Context m_ssl ;
	std::string m_peer_certificate ;
	std::string m_peer_certificate_chain ;
	bool m_verified ;
} ;

//| \class GSsl::MbedTls::DigesterImp
/// An implementation of the GSsl::DigesterImpBase interface for MbedTls.
///
class GSsl::MbedTls::DigesterImp : public GSsl::DigesterImpBase
{
public:
	using Error = MbedTls::Error ;
	DigesterImp( const std::string & , const std::string & , bool ) ;
	~DigesterImp() override ;

private: // overrides
	void add( G::string_view ) override ;
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
	static void check_ret( int , const char * ) ;

private:
	enum class Type { Md5 , Sha1 , Sha256 } ;
	Type m_hash_type{Type::Md5} ;
	mbedtls_md5_context m_md5{} ;
	mbedtls_sha1_context m_sha1{} ;
	mbedtls_sha256_context m_sha256{} ;
	std::size_t m_block_size{64U} ;
	std::size_t m_value_size{16U} ;
	std::size_t m_state_size{20U} ;
} ;

#endif
