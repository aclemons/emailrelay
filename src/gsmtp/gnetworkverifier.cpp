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
/// \file gnetworkverifier.cpp
///

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
		m_response_timeout(response_timeout) ,
		m_command(Command::VRFY)
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

void GSmtp::NetworkVerifier::verify( Command command , const std::string & mail_to_address ,
	const std::string & mail_from_address , const GNet::Address & client_ip ,
	const std::string & auth_mechanism , const std::string & auth_extra )
{
	m_command = command ;
	if( m_client_ptr.get() == nullptr )
	{
		m_client_ptr.reset( std::make_unique<RequestClient>(
			GNet::ExceptionSink(m_client_ptr,m_es.esrc()),
			"verify" , "" ,
			m_location , m_connection_timeout , m_response_timeout ) ) ;
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

void GSmtp::NetworkVerifier::clientDeleted( const std::string & reason )
{
	G_DEBUG( "GSmtp::NetworkVerifier::clientDeleted: reason=[" << reason << "]" ) ;
	if( !reason.empty() )
	{
		std::string to_address = m_to_address ;
		m_to_address.erase() ;

		VerifierStatus status = VerifierStatus::invalid( to_address ,
			true , "cannot verify" , reason ) ;

		doneSignal().emit( m_command , status ) ;
	}
}

void GSmtp::NetworkVerifier::clientEvent( const std::string & s1 , const std::string & s2 , const std::string & )
{
	G_DEBUG( "GSmtp::NetworkVerifier::clientEvent: [" << s1 << "] [" << s2 << "]" ) ;
	if( s1 == "verify" )
	{
		// parse the output from the remote verifier using pipe-delimited
		// fields based on the script-based verifier interface, but backwards
		//
		G::StringArray parts ;
		G::Str::splitIntoFields( s2 , parts , '|' ) ;
		VerifierStatus status = VerifierStatus::invalid( m_to_address ) ;
		if( !parts.empty() && parts[0U] == "100" )
		{
			status.is_valid = false ;
			status.abort = true ;
		}
		else if( parts.size() >= 2U && parts[0U] == "1" )
		{
			const std::string & address = parts[1U] ;
			status = VerifierStatus::remote( m_to_address , address ) ;
		}
		else if( parts.size() >= 3U && parts[0U] == "0" )
		{
			const std::string & mbox = parts[1U] ;
			const std::string & full_name = parts[2U] ;
			status = VerifierStatus::local( m_to_address , full_name , mbox ) ;
		}
		else if( parts.size() >= 2U && ( parts[0U] == "2" || parts[0U] == "3" ) )
		{
			bool temporary = parts[0U] == "3" ;
			const std::string & response = parts[1U] ;
			std::string reason = parts.size() >= 3 ? parts[2U] : std::string() ;
			status = VerifierStatus::invalid( m_to_address ,
				temporary , response , reason ) ;
		}

		doneSignal().emit( m_command , status ) ;
	}
}

G::Slot::Signal<GSmtp::Verifier::Command,const GSmtp::VerifierStatus&> & GSmtp::NetworkVerifier::doneSignal()
{
	return m_done_signal ;
}

void GSmtp::NetworkVerifier::cancel()
{
	m_to_address.erase() ;
	m_client_ptr.reset() ;
}

