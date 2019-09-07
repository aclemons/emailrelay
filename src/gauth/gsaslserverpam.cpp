//
// Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gsaslserverpam.cpp
//

#include "gdef.h"
#include "gpam.h"
#include "gsaslserverpam.h"
#include "gexception.h"
#include "gstr.h"
#include "glog.h"

namespace GAuth
{
	class PamImp ;
	class SaslServerPamImp ;
}

/// \class GAuth::SaslServerPamImp
/// A private implementation class used by GAuth::SaslServerPam.
///
class GAuth::SaslServerPamImp
{
public:
	SaslServerPamImp( bool valid , const std::string & config , bool allow_apop ) ;
	virtual ~SaslServerPamImp() ;
	bool active() const ;
	bool init( const std::string & mechanism ) ;
	std::string apply( const std::string & pwd , bool & done ) ;
	std::string id() const ;
	bool authenticated() const ;

private:
	SaslServerPamImp( const SaslServerPamImp & ) ;
	void operator=( const SaslServerPamImp & ) ;

private:
	bool m_active ;
	bool m_allow_apop ;
	unique_ptr<PamImp> m_pam ;
} ;

/// \class GAuth::PamImp
/// A private implementation of the G::Pam interface used by
/// GAuth::SaslServerPamImp, which is itself a private implementation
/// class used by GAuth::SaslServerPam.
///
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
	virtual void converse( ItemArray & ) override ;
	virtual void delay( unsigned int usec ) override ;

private:
	PamImp( const PamImp & ) ;
	void operator=( const PamImp & ) ;

private:
	std::string m_app ;
	std::string m_id ;
	std::string m_pwd ;
} ;

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
	// TODO asynchronous implementation of pam delay callback
	// ... but that would require the SaslServer interface be made asynchronous
	// so the result of the apply() (ie. the next challenge) gets delivered
	// via a callback -- the complexity trade-off is not compelling
}

// ==

GAuth::SaslServerPamImp::SaslServerPamImp( bool active , const std::string & /*config*/ , bool allow_apop ) :
	m_active(active) ,
	m_allow_apop(allow_apop)
{
}

GAuth::SaslServerPamImp::~SaslServerPamImp()
{
}

bool GAuth::SaslServerPamImp::active() const
{
	return m_active ;
}

bool GAuth::SaslServerPamImp::init( const std::string & mechanism )
{
	return
		G::Str::upper(mechanism) == "PLAIN" ||
		( m_allow_apop && G::Str::upper(mechanism) == "APOP" ) ;
}

std::string GAuth::SaslServerPamImp::id() const
{
	return m_pam.get() ? m_pam->id() : std::string() ;
}

std::string GAuth::SaslServerPamImp::apply( const std::string & response , bool & done )
{
	// parse the PLAIN response
	std::string sep( 1U , '\0' ) ;
	std::string s = G::Str::tail( response , response.find(sep) , std::string() ) ;
	std::string id = G::Str::head( s , s.find(sep) , std::string() ) ;
	std::string pwd = G::Str::tail( s , s.find(sep) , std::string() ) ;

	m_pam.reset( new PamImp( "emailrelay" , id ) ) ;

	try
	{
		m_pam->apply( pwd ) ;
	}
	catch( G::Pam::Error & e )
	{
		G_WARNING( "GAuth::SaslServer::apply: " << e.what() ) ;
		m_pam.reset() ;
	}
	catch( PamImp::NoPrompt & e )
	{
		G_WARNING( "GAuth::SaslServer::apply: pam error: " << e.what() ) ;
		m_pam.reset() ;
	}

	done = true ; // (only single challenge-response supported)
	return std::string() ; // challenge
}

// ==

GAuth::SaslServerPam::SaslServerPam( const SaslServerSecrets & secrets , const std::string & config , bool allow_apop ) :
	m_imp(new SaslServerPamImp(secrets.valid(),config,allow_apop))
{
}

GAuth::SaslServerPam::~SaslServerPam()
{
	delete m_imp ;
}

std::string GAuth::SaslServerPam::mechanisms( char ) const
{
	return "PLAIN" ;
}

std::string GAuth::SaslServerPam::mechanism() const
{
	return "PLAIN" ;
}

bool GAuth::SaslServerPam::trusted( const GNet::Address & ) const
{
	return false ;
}

bool GAuth::SaslServerPam::active() const
{
	return m_imp->active() ;
}

bool GAuth::SaslServerPam::mustChallenge() const
{
	return false ;
}

bool GAuth::SaslServerPam::init( const std::string & mechanism )
{
	return m_imp->init( mechanism ) ;
}

std::string GAuth::SaslServerPam::initialChallenge() const
{
	return std::string() ;
}

std::string GAuth::SaslServerPam::apply( const std::string & response , bool & done )
{
	return m_imp->apply( response , done ) ;
}

bool GAuth::SaslServerPam::authenticated() const
{
	return !m_imp->id().empty() ;
}

std::string GAuth::SaslServerPam::id() const
{
	return m_imp->id() ;
}

bool GAuth::SaslServerPam::requiresEncryption() const
{
	return true ;
}

