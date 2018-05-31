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
///
/// \file gssl_mbedtls.h
///

#ifndef G_SSL_MBEDTLS__H
#define G_SSL_MBEDTLS__H

#include "gdef.h"
#include "gssl.h"
#include <mbedtls/ssl.h>
#include <mbedtls/ssl_ciphersuites.h>
#include <mbedtls/entropy.h>
#if GCONFIG_HAVE_MBEDTLS_NET_H
#include <mbedtls/net.h>
#else
#include <mbedtls/net_sockets.h>
#endif
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/certs.h>
#include <mbedtls/error.h>
#include <mbedtls/version.h>
#include <mbedtls/pem.h>
#include <mbedtls/base64.h>
#include <mbedtls/debug.h>
#include <mbedtls/md5.h>
#include <mbedtls/sha1.h>
#include <mbedtls/sha256.h>
#include <mbedtls/sha512.h>
#include <vector>
#include <map>

namespace GSsl
{
	namespace MbedTls
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

/// \class GSsl::MbedTls::Certificate
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

private:
	Certificate( const Certificate & ) ;
	void operator=( const Certificate & ) ;

private:
	bool m_loaded ;
	mbedtls_x509_crt x ;
} ;

/// \class GSsl::MbedTls::Rng
/// Holds a mbedtls_ctr_drbg_context structure.
///
class GSsl::MbedTls::Rng
{
public:
	Rng() ;
	~Rng() ;
	mbedtls_ctr_drbg_context * ptr() ;
	mbedtls_ctr_drbg_context * ptr() const ;

private:
	Rng( const Rng & ) ;
	void operator=( const Rng & ) ;

private:
	mbedtls_ctr_drbg_context x ; // "counter-mode deterministic random byte generator"
	mbedtls_entropy_context entropy ;
} ;

/// \class GSsl::MbedTls::Key
/// Holds a mbedtls_pk_context structure.
///
class GSsl::MbedTls::Key
{
public:
	Key() ;
	~Key() ;
	void load( const std::string & file ) ;
	mbedtls_pk_context * ptr() ;
	mbedtls_pk_context * ptr() const ;

private:
	Key( const Key & ) ;
	void operator=( const Key & ) ;

private:
	mbedtls_pk_context x ;
} ;

/// \class GSsl::MbedTls::Context
/// Holds a mbedtls_ssl_context structure.
///
class GSsl::MbedTls::Context
{
public:
	explicit Context( const mbedtls_ssl_config * ) ;
	~Context() ;
	mbedtls_ssl_context * ptr() ;
	mbedtls_ssl_context * ptr() const ;

private:
	Context( const Context & ) ;
	void operator=( const Context & ) ;

private:
	mbedtls_ssl_context x ;
} ;

/// \class GSsl::MbedTls::Error
/// An exception class for GSsl::MbedTls classes.
///
class GSsl::MbedTls::Error : public std::exception
{
public:
	explicit Error( const std::string & ) ;
	Error( const std::string & , int rc , const std::string & more = std::string() ) ;
	virtual ~Error() g__noexcept ;
	virtual const char * what() const g__noexcept override ;

private:
	std::string m_what ;
} ;

/// \class GSsl::MbedTls::SecureFile
/// An interface for reading a sensitive file and then overwriting
/// its contents in memory.
///
class GSsl::MbedTls::SecureFile
{
public:
	SecureFile( const std::string & path , bool with_nul ) ;
	~SecureFile() ;
	const char * p() const ;
	const unsigned char * pu() const ;
	unsigned char * pu() ;
	size_t size() const ;
	bool empty() const ;

private:
	SecureFile( const SecureFile & ) ;
	void operator=( const SecureFile & ) ;

private:
	std::vector<char> m_buffer ;
} ;

/// \class GSsl::MbedTls::Config
/// Holds protocol version information, etc.
///
class GSsl::MbedTls::Config
{
public:
	explicit Config( G::StringArray & config ) ;
	int min_() const ;
	int max_() const ;
	bool noverify() const ;

private:
	static bool consume( G::StringArray & , const std::string & ) ;

private:
	bool m_noverify ;
	int m_min ;
	int m_max ;
} ;

/// \class GSsl::MbedTls::LibraryImp
/// An implementation of the GSsl::LibraryImpBase interface for mbedtls.
///
class GSsl::MbedTls::LibraryImp : public LibraryImpBase
{
public:
	typedef MbedTls::Rng Rng ;

	LibraryImp( G::StringArray & , Library::LogFn , bool verbose ) ;
	virtual ~LibraryImp() ;
	virtual void addProfile( const std::string & profile_name , bool is_server_profile ,
		const std::string & key_file , const std::string & cert_file , const std::string & ca_file ,
		const std::string & default_peer_certificate_name , const std::string & default_peer_host_name ,
		const std::string & profile_config ) override ;
	virtual bool hasProfile( const std::string & profile_name ) const override ;
	virtual const Profile & profile( const std::string & profile_name ) const override ;
	virtual std::string id() const override ;
	virtual G::StringArray digesters( bool ) const override ;
	virtual Digester digester( const std::string & , const std::string & ) const override ;
	const Rng & rng() const ;
	Library::LogFn log() const ;
	Config config() const ;
	static std::string credit( const std::string & , const std::string & , const std::string & ) ;
	static std::string sid() ;

private:
	LibraryImp( const LibraryImp & ) ;
	void operator=( const LibraryImp & ) ;
	static int minVersionFrom( G::StringArray & ) ;
	static int maxVersionFrom( G::StringArray & ) ;

private:
	typedef std::map<std::string,shared_ptr<ProfileImp> > Map ;
	Library::LogFn m_log_fn ;
	Config m_config ;
	Map m_profile_map ;
	Rng m_rng ;
	bool m_verbose ;
} ;

/// \class GSsl::MbedTls::ProfileImp
/// An implementation of the GSsl::Profile interface for mbedtls.
///
class GSsl::MbedTls::ProfileImp : public Profile
{
public:
	typedef MbedTls::Key Key ;
	typedef MbedTls::Error Error ;
	typedef MbedTls::Certificate Certificate ;

	ProfileImp( const LibraryImp & library_imp , bool is_server , const std::string & key_file ,
		const std::string & cert_file , const std::string & ca_file ,
		const std::string & default_peer_certificate_name , const std::string & default_peer_host_name ,
		const std::string & profile_config ) ;
	virtual ~ProfileImp() ;
	virtual ProtocolImpBase * newProtocol( const std::string & , const std::string & ) const override ;
	const mbedtls_ssl_config * config() const ;
	mbedtls_x509_crl * crl() const ;
	void logAt( int level_out , std::string ) const ;
	const std::string & defaultPeerCertificateName() const ;
	const std::string & defaultPeerHostName() const ;

private:
	ProfileImp( const ProfileImp & ) ;
	void operator=( const ProfileImp & ) ;
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
} ;

/// \class GSsl::MbedTls::ProtocolImp
/// An implementation of the GSsl::ProtocolImpBase interface for mbedtls.
///
class GSsl::MbedTls::ProtocolImp : public ProtocolImpBase
{
public:
	typedef Protocol::Result Result ;
	typedef MbedTls::Context Context ;
	typedef MbedTls::Error Error ;

	ProtocolImp( const ProfileImp & , const std::string & , const std::string & ) ;
	virtual ~ProtocolImp() ;

	virtual Result connect( G::ReadWrite & ) override ;
	virtual Result accept( G::ReadWrite & ) override ;
	virtual Result read( char * buffer , size_t buffer_size_in , ssize_t & data_size_out ) override ;
	virtual Result write( const char * buffer , size_t data_size_in , ssize_t & data_size_out ) override ;
	virtual Result stop() override ;
	virtual std::string peerCertificate() const override ;
	virtual std::string peerCertificateChain() const override ;
	virtual bool verified() const override ;

	static int doSend( void * , const unsigned char * , size_t ) ;
	static int doRecv( void * , unsigned char * , size_t ) ;
	static int doRecvTimeout( void * , unsigned char * , size_t , uint32_t ) ;
	std::string protocol() const ;
	std::string cipher() const ;
	const Profile & profile() const ;

private:
	ProtocolImp( const ProtocolImp & ) ;
	void operator=( const ProtocolImp & ) ;
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

/// \class GSsl::MbedTls::DigesterImp
/// An implementation of the GSsl::DigesterImpBase interface for MbedTls.
///
class GSsl::MbedTls::DigesterImp : public GSsl::DigesterImpBase
{
public:
	DigesterImp( const std::string & , const std::string & ) ;
	virtual ~DigesterImp() ;
	virtual void add( const std::string & ) override ;
	virtual std::string value() override ;
	virtual std::string state() override ;
	virtual size_t blocksize() const override ;
	virtual size_t valuesize() const override ;
	virtual size_t statesize() const override ;

private:
	DigesterImp( const DigesterImp & ) ;
	void operator=( const DigesterImp & ) ;

private:
	std::string m_hash_type ;
	mbedtls_md5_context m_md5 ;
	mbedtls_sha1_context m_sha1 ;
	mbedtls_sha256_context m_sha256 ;
} ;

#endif
