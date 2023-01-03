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
#include "gstr.h"
#include "glog.h"

GVerifiers::UserVerifier::UserVerifier( GNet::ExceptionSink es , bool local ,
	const GSmtp::Verifier::Config & config , const std::string & spec ,
	bool allow_postmaster ) :
		m_command(Command::RCPT) ,
		m_local(local) ,
		m_config(config) ,
		m_timer(*this,&UserVerifier::onTimeout,es) ,
		m_result(GSmtp::VerifierStatus::invalid({})) ,
		m_range(none()) ,
		m_allow_postmaster(allow_postmaster)
{
	if( local )
	{
		// outgoing messages -- always allow but local if a match
		m_range = spec.empty() ? from(512) : range(spec) ;
		G_DEBUG( "GVerifiers::UserVerifier: verifying uid " << str(m_range) << " as local" ) ;
	}
	else
	{
		// incoming messages -- allow only if a match
		m_range = spec.empty() ? from(512) : range(spec) ;
		G_DEBUG( "GVerifiers::UserVerifier: verifying uid " << str(m_range) << " as valid" ) ;
	}
}

GVerifiers::UserVerifier::~UserVerifier()
= default ;

void GVerifiers::UserVerifier::check( const std::string & spec )
{
	if( !spec.empty() )
	{
		auto r = range( spec ) ; // throws on error
		if( r.first != -1 && r.second < r.first ) // eg. "1000-900"
			throw std::runtime_error( "not a valid uid range" ) ;
	}
}

void GVerifiers::UserVerifier::verify( Command command , const std::string & rcpt_to_parameter ,
	const GSmtp::Verifier::Info & )
{
	m_command = command ;
	std::string user = normalise( G::Str::head( rcpt_to_parameter , "@" , false ) ) ;
	std::string domain = normalise( G::Str::tail( rcpt_to_parameter , "@" ) ) ;
	int uid = -1 ;
	if( m_local )
	{
		// outgoing messages -- always allow but local if a match
		if( m_allow_postmaster && user == "postmaster" && domain == m_config.domain ) // (not useful)
			m_result = GSmtp::VerifierStatus::local( rcpt_to_parameter , {} , G::Str::join("@"_sv,"postmaster"_sv,domain) ) ;
		else if( domain == m_config.domain && (uid=lookup(user)) >= 0 && within(m_range,uid) )
			m_result = GSmtp::VerifierStatus::local( rcpt_to_parameter , {} , std::string(user).append(1U,'@').append(domain) ) ;
		else
			m_result = GSmtp::VerifierStatus::remote( rcpt_to_parameter ) ;
	}
	else
	{
		// incoming messages -- allow only if a match
		if( m_allow_postmaster && user == "postmaster" && domain == m_config.domain ) // (or any domain?)
			m_result = GSmtp::VerifierStatus::remote( rcpt_to_parameter ) ;
		else if( domain == m_config.domain && (uid=lookup(user)) >= 0 && within(m_range,uid) )
			m_result = GSmtp::VerifierStatus::remote( rcpt_to_parameter ) ;
		else
			m_result = GSmtp::VerifierStatus::invalid( rcpt_to_parameter , false , "rejected" , explain(uid,domain,m_config.domain) ) ;
	}
	m_timer.startTimer( 0U ) ;
}

std::string GVerifiers::UserVerifier::explain( int uid , const std::string & domain , const std::string & this_domain ) const
{
	std::ostringstream ss ;
	ss << "not a valid user" ;
	if( domain != this_domain )
		ss << " at [" << this_domain << "]" ;
	if( uid >= 0 )
		ss << ": uid " << uid << " is not in the range " << str(m_range) ;
	return ss.str() ;
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

std::pair<int,int> GVerifiers::UserVerifier::range( const std::string & spec_part )
{
	G_ASSERT( !spec_part.empty() ) ;
	if( spec_part.find('-') == std::string::npos )
	{
		int value = static_cast<int>( G::Str::toUInt( spec_part ) ) ;
		return { value , value } ;
	}
	else
	{
		return {
			static_cast<int>(G::Str::toUInt(G::Str::head(spec_part,"-",false))) ,
			static_cast<int>(G::Str::toUInt(G::Str::tail(spec_part,"-",true)))
		} ;
	}
}

std::string GVerifiers::UserVerifier::str( std::pair<int,int> range )
{
	return G::Str::fromInt(range.first).append(1U,'-').append(G::Str::fromInt(range.second<0?9999:range.second)) ;
}

bool GVerifiers::UserVerifier::within( std::pair<int,int> range , int uid )
{
	return uid >= 0 && uid >= range.first && ( range.second < 0 || uid <= range.second ) ;
}

int GVerifiers::UserVerifier::lookup( const std::string & user )
{
	uid_t uid = 0 ;
	gid_t gid = 0 ;
	bool ok = G::Identity::lookupUser( user , uid , gid ) ;
	return ok ? static_cast<int>(uid) : -1 ;
}

std::string GVerifiers::UserVerifier::normalise( const std::string & s )
{
	return G::Str::isPrintableAscii(s) ? G::Str::lower(s) : s ;
}

