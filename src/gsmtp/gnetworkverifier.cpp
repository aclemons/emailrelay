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
// gnetworkverifier.cpp
//

#include "gdef.h"
#include "gsmtp.h"
#include "gnetworkverifier.h"
#include "glocal.h"
#include "gstr.h"
#include "glog.h"

GSmtp::NetworkVerifier::NetworkVerifier( const std::string & server , 
	unsigned int connection_timeout , unsigned int response_timeout ) :
		m_resolver_info(server) ,
		m_connection_timeout(connection_timeout) ,
		m_response_timeout(response_timeout) ,
		m_lazy(true)
{
	G_DEBUG( "GSmtp::NetworkVerifier::ctor: " << server ) ;
	m_client.eventSignal().connect( G::slot(*this,&GSmtp::NetworkVerifier::clientEvent) ) ;
}

GSmtp::NetworkVerifier::~NetworkVerifier()
{
	m_client.eventSignal().disconnect() ;
}

void GSmtp::NetworkVerifier::verify( const std::string & to ,
		const std::string & mail_from_parameter , const GNet::Address & client_ip ,
		const std::string & auth_mechanism , const std::string & auth_extra )
{
	if( !m_lazy || m_client.get() == NULL )
	{
		m_client.reset( new RequestClient("verify","","\n",m_resolver_info,m_connection_timeout,m_response_timeout) ) ;
	}

	G::Strings args ;
	args.push_back( to ) ;
	args.push_back( G::Str::upper(G::Str::head(to,to.find('@'),to)) ) ;
	args.push_back( G::Str::upper(G::Str::tail(to,to.find('@'),std::string())) ) ;
	args.push_back( G::Str::upper(GNet::Local::fqdn()) ) ;
	args.push_back( mail_from_parameter ) ;
	args.push_back( client_ip.displayString(false) ) ;
	args.push_back( auth_mechanism ) ;
	args.push_back( auth_extra ) ;

	m_to = to ;
	m_client->request( G::Str::join(args,"|") ) ;
}

void GSmtp::NetworkVerifier::clientEvent( std::string s1 , std::string s2 )
{
	G_DEBUG( "GSmtp::NetworkVerifier::clientEvent: [" << s1 << "] [" << s2 << "]" ) ;
	if( s1 == "verify" )
	{
		VerifierStatus status ;

		// parse the output from the remote verifier using pipe-delimited
		// fields based on the script-based verifier interface but backwards
		//
		G::StringArray part ;
		G::Str::splitIntoFields( s2 , part , "|" ) ;
		if( part.size() >= 1U && part[0U] == "100" )
		{
			status.is_valid = false ;
			status.reason = "abort request" ; // TODO -- make this drop the connection
			status.temporary = false ;
		}
		else if( part.size() >= 2U && part[0U] == "1" )
		{
			status.is_valid = true ;
			status.is_local = false ;
			status.address = part[1U] ;
		}
		else if( part.size() >= 3U && part[0U] == "0" )
		{
			status.is_valid = true ;
			status.is_local = true ;
			status.address = part[1U] ;
			status.full_name = part[2U] ;
		}
		else if( part.size() >= 2U && ( part[0U] == "2" || part[0U] == "3" ) )
		{
			status.is_valid = false ;
			status.reason = part[1U] ;
			status.temporary = part[0U] == "3" ;
		}
		else
		{
			status.is_valid = false ;
			status.reason = "external verifier protocol error" ;
			status.temporary = false ;
		}

		doneSignal().emit( G::Str::printable(m_to) , status ) ;
	}
}

G::Signal2<std::string,GSmtp::VerifierStatus> & GSmtp::NetworkVerifier::doneSignal()
{
	return m_done_signal ;
}

void GSmtp::NetworkVerifier::reset()
{
	m_client.reset() ; // kiss
	m_to.erase() ;
}

/// \file gnetworkverifier.cpp
