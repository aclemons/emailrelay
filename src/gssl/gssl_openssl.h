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
/// \file gssl_openssl.h
///

#ifndef G_SSL_OPENSSL__H
#define G_SSL_OPENSSL__H

#include "gdef.h"
#include "gssl.h"
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <map>

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
	namespace OpenSSL
	{
		class Error ;
		class Certificate ;
		class CertificateChain ;
		class LibraryImp ;
		class ProfileImp ;
		class ProtocolImp ;
		class DigesterImp ;
	}
}

/// \class GSsl::OpenSSL::Certificate
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

/// \class GSsl::OpenSSL::CertificateChain
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

/// \class GSsl::OpenSSL::Error
/// An exception class for GSsl::OpenSSL classes.
///
class GSsl::OpenSSL::Error : public std::exception
{
public:
	explicit Error( const std::string & ) ;
	Error( const std::string & , unsigned long ) ;
	Error( const std::string & , unsigned long , const std::string & path ) ;
	virtual ~Error() g__noexcept ;
	virtual const char * what() const g__noexcept override ;
	static void clearErrors() ;

private:
	static std::string text( unsigned long ) ;

private:
	std::string m_what ;
} ;

/// \class GSsl::OpenSSL::ProfileImp
/// An implementation of the GSsl::Profile interface for OpenSSL.
///
class GSsl::OpenSSL::ProfileImp : public Profile
{
public:
	typedef OpenSSL::Error Error ;

	ProfileImp( const LibraryImp & , bool is_server_profile , const std::string & key_file ,
		const std::string & cert_file , const std::string & ca_file ,
		const std::string & default_peer_certificate_name , const std::string & default_peer_host_name ,
		const std::string & library_config , const std::string & profile_config ) ;
	virtual ~ProfileImp() ;
	SSL_CTX * p() const ;
	const LibraryImp & lib() const ;
	const std::string & defaultPeerCertificateName() const ;
	const std::string & defaultPeerHostName() const ;
	virtual ProtocolImpBase * newProtocol( const std::string & , const std::string & ) const override ;

private:
	ProfileImp( const ProfileImp & ) ;
	void operator=( const ProfileImp & ) ;
	static void check( int , const std::string & , const std::string & = std::string() ) ;
	static int verifyPass( int , X509_STORE_CTX * ) ;
	static int verifyPeerName( int , X509_STORE_CTX * ) ;
	static std::string name( X509_NAME * ) ;
	static void deleter( SSL_CTX * ) ;

private:
	const LibraryImp & m_library_imp ;
	const std::string m_default_peer_certificate_name ;
	const std::string m_default_peer_host_name ;
	unique_ptr<SSL_CTX,std::pointer_to_unary_function<SSL_CTX*,void> > m_ssl_ctx ;
} ;

/// \class GSsl::OpenSSL::LibraryImp
/// An implementation of the GSsl::LibraryImpBase interface for OpenSSL.
///
class GSsl::OpenSSL::LibraryImp : public LibraryImpBase
{
public:
	typedef GSsl::OpenSSL::Error Error ;

	LibraryImp( const std::string & library_config , Library::LogFn , bool verbose ) ;
	virtual ~LibraryImp() ;
	virtual void addProfile( const std::string & name , bool is_server_profile ,
		const std::string & key_file , const std::string & cert_file , const std::string & ca_file ,
		const std::string & default_peer_certificate_name , const std::string & default_peer_host_name ,
		const std::string & profile_config ) override ;
	virtual bool hasProfile( const std::string & ) const override ;
	virtual const GSsl::Profile & profile( const std::string & ) const override ;
	virtual std::string id() const override ;
	virtual G::StringArray digesters( bool ) const override ;
	virtual Digester digester( const std::string & , const std::string & ) const override ;

	Library::LogFn log() const ;
	bool verbose() const ;
	int index() const ;
	static std::string credit( const std::string & prefix , const std::string & eol , const std::string & eot ) ;
	static std::string sid() ;

private:
	LibraryImp( const LibraryImp & ) ;
	void operator=( const LibraryImp & ) ;
	static void cleanup() ;

private:
	typedef std::map<std::string,shared_ptr<ProfileImp> > Map ;
	std::string m_library_config ;
	mutable EVP_MD_CTX * m_evp_ctx ;
	Library::LogFn m_log_fn ;
	bool m_verbose ;
	Map m_profile_map ;
	int m_index ; // SSL_get_ex_new_index()
} ;

/// \class GSsl::OpenSSL::ProtocolImp
/// An implementation of the GSsl::ProtocolImpBase interface for OpenSSL.
///
class GSsl::OpenSSL::ProtocolImp : public ProtocolImpBase
{
public:
	typedef Protocol::Result Result ;
	typedef Protocol::size_type size_type ;
	typedef Protocol::ssize_type ssize_type ;
	typedef OpenSSL::Error Error ;
	typedef OpenSSL::Certificate Certificate ;
	typedef OpenSSL::CertificateChain CertificateChain ;

	ProtocolImp( const ProfileImp & , const std::string & , const std::string & ) ;
	virtual ~ProtocolImp() ;
	Result connect( G::ReadWrite & ) ;
	Result accept( G::ReadWrite & ) ;
	Result stop() ;
	Result read( char * buffer , size_type buffer_size , ssize_type & read_size ) ;
	Result write( const char * buffer , size_type size_in , ssize_type & size_out ) ;
	std::string peerCertificate() const ;
	std::string peerCertificateChain() const ;
	bool verified() const ;
	std::string requiredPeerCertificateName() const ;

private:
	ProtocolImp( const ProtocolImp & ) ;
	void operator=( const ProtocolImp & ) ;
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
	const Profile & m_profile ;
	unique_ptr<SSL,std::pointer_to_unary_function<SSL*,void> > m_ssl ;
	Library::LogFn m_log_fn ;
	bool m_verbose ;
	bool m_fd_set ;
	std::string m_required_peer_certificate_name ;
	std::string m_peer_certificate ;
	std::string m_peer_certificate_chain ;
	bool m_verified ;
} ;

/// \class GSsl::OpenSSL::DigesterImp
/// An implementation of the GSsl::DigesterImpBase interface for OpenSSL.
///
class GSsl::OpenSSL::DigesterImp : public GSsl::DigesterImpBase
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
	EVP_MD_CTX * m_evp_ctx ;
	size_t m_block_size ;
	size_t m_value_size ;
	size_t m_state_size ;
} ;

#endif
