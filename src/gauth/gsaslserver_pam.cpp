//
// Copyright (C) 2001-2008 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gsaslserver_pam.cpp
//
// This tries to match up the PAM interface with the 
// SASL server interface. The match is not good; only
// single-challenge PAM mechanisms are supported,
// the PAM delay feature is not implemented, and
// PAM sessions are not part of the SASL interface.
//

#include "gdef.h"
#include "gnet.h"
#include "gauth.h"
#include "gpam.h"
#include "gsaslserver.h"
#include "gexception.h"
#include "gstr.h"

namespace GAuth
{
	class PamImp ;
}

class GAuth::PamImp : public G::Pam 
{
public:
	typedef GAuth::PamImp::ItemArray ItemArray ;
	G_EXCEPTION_CLASS( NoPrompt , "no password prompt received from pam module" ) ;

	PamImp( const std::string & app , const std::string & id ) ;
	virtual ~PamImp() ;
	void fail() ;
	void apply( const std::string & ) ;
	std::string id() const ;

protected:
	virtual void converse( ItemArray & ) ;
	virtual void delay( unsigned int usec ) ;

private:
	PamImp( const PamImp & ) ;
	void operator=( const PamImp & ) ;

private:
	std::string m_app ;
	std::string m_id ;
	std::string m_pwd ;
} ;

// ==

class GAuth::SaslServerImp 
{
public:
	explicit SaslServerImp( bool valid ) ;
	virtual ~SaslServerImp() ;
	bool active() const ;
	std::string apply( const std::string & pwd , bool & done ) ;
	std::string id() const ;
	bool authenticated() const ;

private:
	SaslServerImp( const SaslServerImp & ) ;
	void operator=( const SaslServerImp & ) ;

private:
	bool m_active ;
	PamImp * m_pam ;
} ;

// ==

GAuth::PamImp::PamImp( const std::string & app , const std::string & id ) :
	G::Pam(app,id,true) ,
	m_app(app) ,
	m_id(id)
{
	G_DEBUG( "GAuth::PamImp::ctor: [" << app << "] [" << id << "]" ) ;
}

GAuth::PamImp::~PamImp()
{
}

std::string GAuth::PamImp::id() const
{
	return m_id ;
}

void GAuth::PamImp::converse( ItemArray & items )
{
	bool done = false ;
	for( ItemArray::iterator p = items.begin() ; p != items.end() ; ++p )
	{
		if( (*p).in_type == "password" )
		{
			(*p).out = m_pwd ;
			(*p).out_defined = true ;
			done = true ;
		}
	}
	if( !done )
	{
		throw NoPrompt() ;
	}
}

void GAuth::PamImp::apply( const std::string & pwd )
{
	m_pwd = pwd ;
	authenticate( true ) ; // base class -- calls converse() -- thows on error
}

void GAuth::PamImp::delay( unsigned int )
{
	// TODO
}

// ==

GAuth::SaslServerImp::SaslServerImp( bool active ) :
	m_active(active) ,
	m_pam(NULL)
{
}

GAuth::SaslServerImp::~SaslServerImp()
{
	delete m_pam ;
}

bool GAuth::SaslServerImp::active() const
{
	return m_active ;
}

std::string GAuth::SaslServerImp::id() const
{
	return m_pam ? m_pam->id() : std::string() ;
}

std::string GAuth::SaslServerImp::apply( const std::string & response , bool & done )
{
	// parse the PLAIN response
	std::string sep( 1U , '\0' ) ;
	std::string s = G::Str::tail( response , response.find(sep) , std::string() ) ;
	std::string id = G::Str::head( s , s.find(sep) , std::string() ) ;
	std::string pwd = G::Str::tail( s , s.find(sep) , std::string() ) ;

	delete m_pam ;
	m_pam = NULL ;
	m_pam = new PamImp( "emailrelay" , id ) ;

	try
	{
		m_pam->apply( pwd ) ;
	}
	catch( G::Pam::Error & e )
	{
		G_WARNING( "GAuth::SaslServer::apply: " << e.what() ) ;
		delete m_pam ;
		m_pam = NULL ;
	}
	catch( PamImp::NoPrompt & e )
	{
		G_WARNING( "GAuth::SaslServer::apply: pam error: " << e.what() ) ;
		delete m_pam ;
		m_pam = NULL ;
	}

	done = true ; // (only single challenge-response supported)
	return std::string() ; // challenge
}

// ==

GAuth::SaslServer::SaslServer( const SaslServer::Secrets & secrets , bool , bool ) :
	m_imp(new SaslServerImp(secrets.valid()))
{
}

GAuth::SaslServer::~SaslServer()
{
	delete m_imp ;
}

std::string GAuth::SaslServer::mechanisms( char ) const
{
	return "PLAIN" ;
}

std::string GAuth::SaslServer::mechanism() const
{
	return "PLAIN" ;
}

bool GAuth::SaslServer::trusted( GNet::Address ) const
{
	return false ;
}

bool GAuth::SaslServer::active() const
{
	return m_imp->active() ;
}

bool GAuth::SaslServer::mustChallenge() const
{
	return false ;
}

bool GAuth::SaslServer::init( const std::string & mechanism )
{
	return mechanism == "PLAIN" ;
}

std::string GAuth::SaslServer::initialChallenge() const
{
	return std::string() ;
}

std::string GAuth::SaslServer::apply( const std::string & response , bool & done )
{
	return m_imp->apply( response , done ) ;
}

bool GAuth::SaslServer::authenticated() const
{
	return !m_imp->id().empty() ;
}

std::string GAuth::SaslServer::id() const
{
	return m_imp->id() ;
}

bool GAuth::SaslServer::requiresEncryption() const
{
	return true ;
}

// ==

GAuth::SaslServer::Secrets::~Secrets()
{
}

/// \file gsaslserver_pam.cpp
