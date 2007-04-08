//
// Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later
// version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
// ===
///
/// \file gsasl.h
///

#ifndef G_SASL_H
#define G_SASL_H

#include "gdef.h"
#include "gsmtp.h"
#include "gexception.h"
#include "gaddress.h"
#include "gstrings.h"
#include "gpath.h"
#include <map>
#include <memory>

/// \namespace GSmtp
namespace GSmtp
{
	class SaslClient ;
	class SaslClientImp ;
	class SaslServer ;
	class SaslServerImp ;
	class Valid ;
}

/// \class GSmtp::Valid
/// A mix-in interface containing a valid() method.
class GSmtp::Valid 
{
public:
	virtual bool valid() const = 0 ;
		///< Returns true if a valid source of information.

	virtual ~Valid() ;
		///< Destructor.
} ;

/// \class GSmtp::SaslServer
/// A class for implementing the server-side SASL 
/// challenge/response concept. SASL is described in RFC2222,
/// and the SMTP extension for authentication is described 
/// in RFC2554.
///
/// Common SASL mechanisms are:
/// - GSSAPI [RFC2222]
/// - CRAM-MD5 [RFC2195]
/// - PLAIN [RFC2595]
/// - DIGEST-MD5 [RFC2831]
/// - KERBEROS_V5
/// - LOGIN
///
/// Usage:
/// \code
/// SaslServer sasl( secrets ) ;
/// client.advertise( sasl.mechanisms() ) ;
/// if( sasl.init(client.preferredMechanism()) )
/// {
///   client.send( sasl.initialChallenge() ) ;
///   for(;;)
///   {
///     std::string reply = client.receive() ;
///     bool done = false ;
///     std::string challenge = sasl.apply( reply , done ) ;
///     if( done ) break ;
///     client.send( challenge ) ;
///   }
///   bool ok = sasl.authenticated() ;
/// }
/// \endcode
///
/// \see GSmtp::SaslClient, RFC2554, RFC2222
///
class GSmtp::SaslServer 
{
public:
	/// An interface used by GSmtp::SaslServer to obtain authentication secrets.
	class Secrets : public virtual Valid 
	{
		public: virtual std::string secret( const std::string & mechanism, const std::string & id ) const = 0 ;
		public: virtual ~Secrets() ;
		public: virtual bool contains( const std::string & mechanism ) const = 0 ;
		private: void operator=( const Secrets & ) ; // not implemented
	} ;

	SaslServer( const Secrets & , bool strict , bool force_one_mechanism ) ;
		///< Constructor. The secrets reference is kept.
		///<
		///< If the 'strict' flag is false then LOGIN is treated 
		///< as a standard mechanism and therefore advertised
		///< by mechanisms().
		///<
		///< If the 'force' flag is true then the list of
		///< mechanisms returned by mechanisms() will never
		///< be empty, even if no authentication is possible.

	~SaslServer() ;
		///< Destructor.

	bool active() const ;
		///< Returns true if the constructor's "secrets" object 
		///< was valid. See also Secrets::valid().

	std::string mechanisms( char sep = ' ' ) const ;
		///< Returns a list of supported, standard mechanisms
		///< that can be advertised to the client.
		///<
		///< Mechanisms (eg. APOP) may still be accepted by 
		///< init() even though they are not advertised.

	bool init( const std::string & mechanism ) ;
		///< Initialiser. Returns true if a supported mechanism.
		///< May be used more than once.

	std::string mechanism() const ;
		///< Returns the mechanism, as passed to the last init()
		///< call to return true.

	bool mustChallenge() const ;
		///< Returns true if the mechanism must start with
		///< a non-empty server challenge. Returns false for
		///< the "LOGIN" mechanism since the initial challenge
		///< ("username:") is not essential.

	std::string initialChallenge() const ;
		///< Returns the initial server challenge. May return
		///< an empty string.

	std::string apply( const std::string & response , bool & done ) ;
		///< Applies the client response and returns the 
		///< next challenge.

	bool authenticated() const ;
		///< Returns true if authenticated sucessfully.
		///< Precondition: apply() returned empty

	std::string id() const ;
		///< Returns the authenticated or trusted identity. Returns the
		///< empty string if not authenticated and not trusted.

	bool trusted( GNet::Address ) const ;
		///< Returns true if a trusted client that
		///< does not need to authenticate.

private:
	SaslServer( const SaslServer & ) ; // not implemented
	void operator=( const SaslServer & ) ; // not implemented

private:
	SaslServerImp * m_imp ;
} ;

/// \class GSmtp::SaslClient
/// A class for implementing the client-side SASL 
/// challenge/response concept. SASL is described in RFC2222,
/// and the SMTP extension for authentication is described 
/// in RFC2554.
/// \see GSmtp::SaslServer, RFC2222, RFC2554.
///
class GSmtp::SaslClient 
{
public:
	/// An interface used by GSmtp::SaslClient to obtain authentication secrets.
	class Secrets : public virtual Valid 
	{
		public: virtual std::string id( const std::string & mechanism ) const = 0 ;
		public: virtual std::string secret( const std::string & id ) const = 0 ;
		public: virtual ~Secrets() ;
		private: void operator=( const Secrets & ) ; // not implemented
	} ;

	SaslClient( const Secrets & secrets , const std::string & server_name ) ;
		///< Constructor. The secrets reference is kept.

	~SaslClient() ;
		///< Destructor.

	bool active() const ;
		///< Returns true if the constructor's secrets object
		///< is valid.

	std::string response( const std::string & mechanism , const std::string & challenge , 
		bool & done , bool & error ) const ;
			///< Returns a response to the given challenge.

	std::string preferred( const G::Strings & mechanisms ) const ;
		///< Returns the name of the preferred mechanism taken from
		///< the given set. Returns the empty string if none is
		///< supported or if not active().

private:
	SaslClient( const SaslClient & ) ; // not implemented
	void operator=( const SaslClient & ) ; // not implemented

private:
	SaslClientImp * m_imp ;
} ;

#endif
