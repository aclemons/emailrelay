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
// gnetworkverifier.cpp
//

#include "gdef.h"
#include "gnetworkverifier.h"
#include "glocal.h"
#include "gstr.h"
#include "glog.h"

GSmtp::NetworkVerifier::NetworkVerifier( GNet::ExceptionSink es ,
	const std::string & server , unsigned int connection_timeout ,
	unsigned int response_timeout ) :
		m_es(es) ,
		m_location(server) ,
		m_connection_timeout(connection_timeout) ,
		m_response_timeout(response_timeout)
{
	G_DEBUG( "GSmtp::NetworkVerifier::ctor: " << server ) ;
	m_client_ptr.eventSignal().connect( G::Slot::slot(*this,&GSmtp::NetworkVerifier::clientEvent) ) ;
	m_client_ptr.deletedSignal().connect( G::Slot::slot(*this,&GSmtp::NetworkVerifier::clientDeleted) ) ;
}

GSmtp::NetworkVerifier::~NetworkVerifier()
{
	m_client_ptr.eventSignal().disconnect() ;
	m_client_ptr.deletedSignal().disconnect() ;
}

void GSmtp::NetworkVerifier::verify( const std::string & mail_to_address ,
		const std::string & mail_from_address , const GNet::Address & client_ip ,
		const std::string & auth_mechanism , const std::string & auth_extra )
{
	if( m_client_ptr.get() == nullptr )
	{
		m_client_ptr.reset( new RequestClient(GNet::ExceptionSink(m_client_ptr,nullptr),"verify","",m_location,m_connection_timeout,m_response_timeout) ) ;
	}

	G::StringArray args ;
	args.push_back( mail_to_address ) ;
	args.push_back( mail_from_address ) ;
	args.push_back( client_ip.displayString() ) ;
	args.push_back( GNet::Local::canonicalName() ) ;
	args.push_back( G::Str::lower(auth_mechanism) ) ;
	args.push_back( auth_extra ) ;

	m_to_address = mail_to_address ;
	m_client_ptr->request( G::Str::join("|",args) ) ;
}

void GSmtp::NetworkVerifier::clientDeleted( std::string reason )
{
	G_DEBUG( "GSmtp::NetworkVerifier::clientDeleted: reason=[" << reason << "]" ) ;
	if( !reason.empty() )
	{
		std::string to_address = m_to_address ;
		m_to_address.erase() ;
		VerifierStatus status( to_address ) ;
		status.is_valid = false ;
		status.temporary = true ;
		status.response = "cannot verify" ;
		status.reason = reason ;
		doneSignal().emit( G::Str::printable(to_address) , status ) ;
	}
}

void GSmtp::NetworkVerifier::clientEvent( std::string s1 , std::string s2 , std::string /*s3*/ )
{
	G_DEBUG( "GSmtp::NetworkVerifier::clientEvent: [" << s1 << "] [" << s2 << "]" ) ;
	if( s1 == "verify" )
	{
		VerifierStatus status ;

		// parse the output from the remote verifier using pipe-delimited
		// fields based on the script-based verifier interface, but backwards
		//
		G::StringArray part ;
		G::Str::splitIntoFields( s2 , part , "|" ) ;
		if( part.size() >= 1U && part[0U] == "100" )
		{
			status.is_valid = false ;
			status.abort = true ;
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
			status.response = part[1U] ;
			status.temporary = part[0U] == "3" ;
		}
		else
		{
			status.is_valid = false ;
			status.temporary = false ;
		}

		doneSignal().emit( G::Str::printable(m_to_address) , status ) ;
	}
}

G::Slot::Signal2<std::string,GSmtp::VerifierStatus> & GSmtp::NetworkVerifier::doneSignal()
{
	return m_done_signal ;
}

void GSmtp::NetworkVerifier::cancel()
{
	m_to_address.erase() ;
	m_client_ptr.reset() ;
}

/// \file gnetworkverifier.cpp
