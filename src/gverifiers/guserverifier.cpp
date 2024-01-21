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
/// \file guserverifier.cpp
///

#include "gdef.h"
#include "guserverifier.h"
#include "gidentity.h"
#include "grange.h"
#include "gstr.h"
#include "gstringtoken.h"
#include "glog.h"
#include <sstream>

GVerifiers::UserVerifier::UserVerifier( GNet::ExceptionSink es ,
	const GSmtp::Verifier::Config & config , const std::string & spec ) :
		m_config(config) ,
		m_timer(*this,&UserVerifier::onTimeout,es) ,
		m_result(GSmtp::VerifierStatus::invalid({})) ,
		m_range(G::Range::range(1000,32767))
{
	G::string_view spec_view( spec ) ;
	for( G::StringTokenView t( spec_view , ";" , 1U ) ; t ; ++t )
	{
		if( !t().empty() && G::Str::isNumeric(t().substr(0U,1U)) )
			m_range = G::Range::range( t() ) ;
		else if( ( t().size() <= 3U && t().find('l') != std::string::npos ) || t() == "lowercase"_sv )
			m_config_lc = true ;
		else if( ( t().size() <= 3U && t().find('r') != std::string::npos ) || t() == "remote"_sv )
			m_config_remote = true ;
		else if( ( t().size() <= 3U && t().find('c') != std::string::npos ) || t() == "check"_sv )
			m_config_check = true ;
	}
	G_DEBUG( "GVerifiers::UserVerifier: uid range " << G::Range::str(m_range) ) ;
}

void GVerifiers::UserVerifier::verify( Command command , const std::string & rcpt_to_parameter ,
	const GSmtp::Verifier::Info & )
{
	using namespace G::Range ; // within()
	m_command = command ;

	std::string user = dequote( G::Str::head( dequote(rcpt_to_parameter) , "@" , false ) ) ;
	std::string domain = G::Str::tail( dequote(rcpt_to_parameter) , "@" ) ;
	std::string reason ;
	std::string mailbox ;

	if( user == "postmaster" && domain.empty() )
		m_result = GSmtp::VerifierStatus::local( rcpt_to_parameter , {} , "postmaster" ) ;
	else if( lookup(user,domain,&reason,&mailbox) )
		m_result =
			m_config_remote ?
				GSmtp::VerifierStatus::remote( rcpt_to_parameter ) :
				GSmtp::VerifierStatus::local( rcpt_to_parameter , {} , m_config_lc?G::Str::lower(mailbox):mailbox ) ;
	else if( m_config_check )
		m_result = GSmtp::VerifierStatus::remote( rcpt_to_parameter ) ;
	else
		m_result = GSmtp::VerifierStatus::invalid( rcpt_to_parameter , false , "rejected" , reason ) ;

	m_timer.startTimer( 0U ) ;
}

bool GVerifiers::UserVerifier::lookup( const std::string & user , const std::string & domain ,
	std::string * reason_p , std::string * mailbox_p ) const
{
	bool result = false ;
	std::ostringstream ss ;
	if( !G::Str::imatch( domain , m_config.domain ) )
	{
		ss << "[" << domain << "] does not match [" << m_config.domain << "]" ;
	}
	else
	{
		auto pair = G::Identity::lookup( user , std::nothrow ) ;

		if( pair.first == G::Identity::invalid() && G::Str::isPrintableAscii(user) && !G::is_windows() )
			pair = G::Identity::lookup( G::Str::lower(user) , std::nothrow ) ;

		if( pair.first == G::Identity::invalid() || pair.second.empty() )
		{
			ss << "[" << user << "] is not a valid account name" ;
		}
		else if( !pair.first.match( m_range ) )
		{
			ss << "uid " << pair.first.userid() << " is not in the range " << G::Range::str(m_range) ;
		}
		else
		{
			if( mailbox_p )
				*mailbox_p = pair.second ;
			result = true ;
		}
	}
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

std::string GVerifiers::UserVerifier::dequote( const std::string & s )
{
	if( s.size() >= 2U && s.at(0) == '"' && s.at(s.size()-1U) == '"' )
		return s.substr( 1U , s.size()-2U ) ;
	else
		return s ;
}

