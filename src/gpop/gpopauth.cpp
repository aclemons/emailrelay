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
// gpopauth.cpp
//

#include "gdef.h"
#include "gpop.h"
#include "gpopauth.h"
#include "gsaslserver.h"
#include "gsaslserverfactory.h"
#include "gstr.h"
#include "gmemory.h"

/// \class GPop::AuthImp
/// A private pimple-pattern implementation class used by GPop::Auth.
class GPop::AuthImp 
{
public:
	explicit AuthImp( const Secrets & ) ;
	bool valid() const ;
	bool init( const std::string & mechanism ) ;
	bool authenticated( const std::string & , const std::string & ) ;
	bool mustChallenge() const ;
	std::string challenge() ;
	std::string id() const ;
	std::string mechanisms() const ;
	bool sensitive() const ;

private:
	const Secrets & m_secrets ;
	std::auto_ptr<GAuth::SaslServer> m_sasl ;
} ;

// ==

GPop::AuthImp::AuthImp( const Secrets & secrets ) :
	m_secrets(secrets) ,
	m_sasl(GAuth::SaslServerFactory::newSaslServer(secrets,true,false)) 
{
	m_sasl->init( "APOP" ) ; // for the initial challenge()
}

bool GPop::AuthImp::valid() const
{
	return m_secrets.valid() && m_sasl->active() ;
}

bool GPop::AuthImp::init( const std::string & mechanism )
{
	G_DEBUG( "GPop::AuthImp::init: mechanism " << mechanism ) ;
	return m_sasl->init(mechanism) ;
}

bool GPop::AuthImp::authenticated( const std::string & p1 , const std::string & p2 )
{
	bool done_1 = false ;
	std::string challenge_1 = m_sasl->apply( p1 , done_1 ) ;
	if( done_1 ) 
	{
		return challenge_1.empty() && m_sasl->authenticated() ;
	}
	else
	{
		bool done_2 = false ;
		std::string challenge_2 = m_sasl->apply( p2 , done_2 ) ;
		return done_2 && challenge_2.empty() && m_sasl->authenticated() ;
	}
}

bool GPop::AuthImp::mustChallenge() const
{
	return m_sasl->mustChallenge() ;
}

std::string GPop::AuthImp::challenge()
{
	return m_sasl->initialChallenge() ;
}

std::string GPop::AuthImp::id() const
{
	return m_sasl->id() ;
}

std::string GPop::AuthImp::mechanisms() const
{
	return m_sasl->mechanisms() ;
}

bool GPop::AuthImp::sensitive() const
{
	return m_sasl->requiresEncryption() ;
}

// ==

GPop::Auth::Auth( const Secrets & secrets ) :
	m_imp( new AuthImp(secrets) )
{
}

GPop::Auth::~Auth()
{
	delete m_imp ;
}

bool GPop::Auth::valid() const
{
	return m_imp->valid() ;
}

bool GPop::Auth::init( const std::string & mechanism )
{
	return m_imp->init(mechanism) ;
}

bool GPop::Auth::authenticated( const std::string & p1 , const std::string & p2 )
{
	return m_imp->authenticated(p1,p2) ;
}

bool GPop::Auth::mustChallenge() const
{
	return m_imp->mustChallenge() ;
}

std::string GPop::Auth::challenge()
{
	return m_imp->challenge() ;
}

std::string GPop::Auth::id() const
{
	return m_imp->id() ;
}

std::string GPop::Auth::mechanisms() const
{
	return m_imp->mechanisms() ;
}

bool GPop::Auth::sensitive() const
{
	return m_imp->sensitive() ;
}
