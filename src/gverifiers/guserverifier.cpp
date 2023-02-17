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
/// \file guserverifier.cpp
///

#include "gdef.h"
#include "guserverifier.h"
#include "gidentity.h"
#include "grange.h"
#include "gstr.h"
#include "glog.h"
#include <sstream>

GVerifiers::UserVerifier::UserVerifier( GNet::ExceptionSink es , bool local ,
	const GSmtp::Verifier::Config & config , const std::string & spec ,
	bool allow_postmaster ) :
		m_command(Command::RCPT) ,
		m_local(local) ,
		m_config(config) ,
		m_timer(*this,&UserVerifier::onTimeout,es) ,
		m_result(GSmtp::VerifierStatus::invalid({})) ,
		m_range(G::Range::none()) ,
		m_allow_postmaster(allow_postmaster)
{
	using namespace G::Range ;
	if( local )
	{
		// outgoing messages -- always allow but local if a match
		m_range = spec.empty() ? range(512,32767) : range(spec) ;
		G_DEBUG( "GVerifiers::UserVerifier: verifying uid " << str(m_range) << " as local" ) ;
	}
	else
	{
		// incoming messages -- allow only if a match
		m_range = spec.empty() ? range(512,32767) : range(spec) ;
		G_DEBUG( "GVerifiers::UserVerifier: verifying uid " << str(m_range) << " as valid" ) ;
	}
}

GVerifiers::UserVerifier::~UserVerifier()
= default ;

void GVerifiers::UserVerifier::verify( Command command , const std::string & rcpt_to_parameter ,
	const GSmtp::Verifier::Info & )
{
	using namespace G::Range ; // within()
	m_command = command ;
	std::string user = normalise( G::Str::head( rcpt_to_parameter , "@" , false ) ) ;
	std::string domain = normalise( G::Str::tail( rcpt_to_parameter , "@" ) ) ;
	if( m_local )
	{
		// outgoing messages -- always allow but local if a match
		if( m_allow_postmaster && user == "postmaster" && domain == m_config.domain ) // (not useful)
			m_result = GSmtp::VerifierStatus::local( rcpt_to_parameter , {} , G::Str::join("@"_sv,"postmaster"_sv,domain) ) ;
		else if( lookup(user,domain) )
			m_result = GSmtp::VerifierStatus::local( rcpt_to_parameter , {} , std::string(user).append(1U,'@').append(domain) ) ;
		else
			m_result = GSmtp::VerifierStatus::remote( rcpt_to_parameter ) ;
	}
	else
	{
		// incoming messages -- allow only if a match
		std::string reason ;
		if( m_allow_postmaster && user == "postmaster" ) // (any domain)
			m_result = GSmtp::VerifierStatus::remote( rcpt_to_parameter ) ;
		else if( lookup(user,domain,&reason) )
			m_result = GSmtp::VerifierStatus::remote( rcpt_to_parameter ) ;
		else
			m_result = GSmtp::VerifierStatus::invalid( rcpt_to_parameter , false , "rejected" , reason ) ;
	}
	m_timer.startTimer( 0U ) ;
}

bool GVerifiers::UserVerifier::lookup( const std::string & user , const std::string & domain ,
	std::string * reason_p ) const
{
	bool result = false ;
	std::ostringstream ss ;
	G::Identity id = G::Identity::invalid() ;
	if( domain != m_config.domain )
		ss << "[" << domain << "] does not match [" << m_config.domain << "]" ;
	else if( (id=G::Identity::lookup(user,std::nothrow)) == G::Identity::invalid() )
		ss << "[" << user << "] is not a valid account name" ;
	else if( !id.match( m_range ) )
		ss << "uid " << id.userid() << " is not in the range " << G::Range::str(m_range) ;
	else
		result = true ;
	if( !result && reason_p ) *reason_p = ss.str() ;
	return result ;
}

GVerifiers::UserVerifier::Signal & GVerifiers::UserVerifier::doneSignal()
{
	return m_done_signal ;
}

void GVerifiers::UserVerifier::cancel()
{
	m_timer.cancelTimer() ;
}

void GVerifiers::UserVerifier::onTimeout()
{
	m_done_signal.emit( m_command , m_result ) ;
}

std::string GVerifiers::UserVerifier::normalise( const std::string & s )
{
	return G::Str::isPrintableAscii(s) ? G::Str::lower(s) : s ;
}

