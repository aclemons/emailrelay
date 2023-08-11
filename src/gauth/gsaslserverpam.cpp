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
/// \file gsaslserverpam.cpp
///

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

//| \class GAuth::SaslServerPamImp
/// A private implementation class used by GAuth::SaslServerPam.
///
class GAuth::SaslServerPamImp
{
public:
	explicit SaslServerPamImp( bool with_apop ) ;
	virtual ~SaslServerPamImp() ;
	G::StringArray mechanisms() const ;
	std::string mechanism() const ;
	void reset() ;
	bool init( bool , const std::string & mechanism ) ;
	std::string apply( const std::string & pwd , bool & done ) ;
	std::string id() const ;
	bool authenticated() const ;

public:
	SaslServerPamImp( const SaslServerPamImp & ) = delete ;
	SaslServerPamImp( SaslServerPamImp && ) = delete ;
	SaslServerPamImp & operator=( const SaslServerPamImp & ) = delete ;
	SaslServerPamImp & operator=( SaslServerPamImp && ) = delete ;

private:
	std::unique_ptr<PamImp> m_pam ;
	G::StringArray m_mechanisms ;
	std::string m_mechanism ;
} ;

//| \class GAuth::PamImp
/// A private implementation of the G::Pam interface used by
/// GAuth::SaslServerPamImp, which is itself a private implementation
/// class used by GAuth::SaslServerPam.
///
class GAuth::PamImp : public G::Pam
{
public:
	using ItemArray = GAuth::PamImp::ItemArray ;
	G_EXCEPTION_CLASS( NoPrompt , tx("no password prompt received from pam module") ) ;

	PamImp( const std::string & app , const std::string & id ) ;
	~PamImp() override ;
	void fail() ;
	void apply( const std::string & ) ;
	std::string id() const ;

protected:
	void converse( ItemArray & ) override ;
	void delay( unsigned int usec ) override ;

public:
	PamImp( const PamImp & ) = delete ;
	PamImp( PamImp && ) = delete ;
	PamImp & operator=( const PamImp & ) = delete ;
	PamImp & operator=( PamImp && ) = delete ;

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
= default;

std::string GAuth::PamImp::id() const
{
	return m_id ;
}

void GAuth::PamImp::converse( ItemArray & items )
{
	bool done = false ;
	for( auto & item : items )
	{
		if( item.in_type == "password" )
		{
			item.out = m_pwd ;
			item.out_defined = true ;
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

GAuth::SaslServerPamImp::SaslServerPamImp( bool with_apop )
{
	m_mechanisms.push_back( "PLAIN" ) ;
	if( with_apop )
		m_mechanisms.push_back( "APOP" ) ;
}

GAuth::SaslServerPamImp::~SaslServerPamImp()
= default;

G::StringArray GAuth::SaslServerPamImp::mechanisms() const
{
	return m_mechanisms ;
}

std::string GAuth::SaslServerPamImp::mechanism() const
{
	return m_mechanism ;
}

void GAuth::SaslServerPamImp::reset()
{
	m_mechanism.clear() ;
	m_pam.reset() ;
}

bool GAuth::SaslServerPamImp::init( bool , const std::string & mechanism )
{
	m_mechanism = G::Str::upper( mechanism ) ;
	return std::find( m_mechanisms.begin() , m_mechanisms.end() , m_mechanism ) != m_mechanisms.end() ;
}

std::string GAuth::SaslServerPamImp::id() const
{
	return m_pam ? m_pam->id() : std::string() ;
}

std::string GAuth::SaslServerPamImp::apply( const std::string & response , bool & done )
{
	// parse the PLAIN response
	std::string sep( 1U , '\0' ) ;
	std::string s = G::Str::tail( response , response.find(sep) , std::string() ) ;
	std::string id = G::Str::head( s , s.find(sep) , std::string() ) ;
	std::string pwd = G::Str::tail( s , s.find(sep) , std::string() ) ;

	m_pam = std::make_unique<PamImp>( "emailrelay" , id ) ;

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

GAuth::SaslServerPam::SaslServerPam( bool with_apop ) :
	m_imp(std::make_unique<SaslServerPamImp>(with_apop))
{
}

GAuth::SaslServerPam::~SaslServerPam()
= default ;

G::StringArray GAuth::SaslServerPam::mechanisms( bool /*secure*/ ) const
{
	return m_imp->mechanisms() ;
}

std::string GAuth::SaslServerPam::mechanism() const
{
	return m_imp->mechanism() ;
}

std::string GAuth::SaslServerPam::preferredMechanism( bool ) const
{
	return std::string() ;
}

bool GAuth::SaslServerPam::trusted( const G::StringArray & , const std::string & ) const
{
	return false ;
}

bool GAuth::SaslServerPam::mustChallenge() const
{
	return false ;
}

void GAuth::SaslServerPam::reset()
{
	return m_imp->reset() ;
}

bool GAuth::SaslServerPam::init( bool secure , const std::string & mechanism )
{
	return m_imp->init( secure , mechanism ) ;
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

