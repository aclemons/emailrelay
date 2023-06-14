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
/// \file gnetworkverifier.cpp
///

#include "gdef.h"
#include "gnetworkverifier.h"
#include "glocal.h"
#include "gstr.h"
#include "gstringfield.h"
#include "glog.h"

GVerifiers::NetworkVerifier::NetworkVerifier( GNet::ExceptionSink es , const GSmtp::Verifier::Config & config ,
	const std::string & server ) :
		m_es(es) ,
		m_location(server) ,
		m_connection_timeout(config.timeout) ,
		m_response_timeout(config.timeout) ,
		m_command(Command::VRFY)
{
	G_DEBUG( "GVerifiers::NetworkVerifier::ctor: " << server ) ;
	m_client_ptr.eventSignal().connect( G::Slot::slot(*this,&GVerifiers::NetworkVerifier::clientEvent) ) ;
	m_client_ptr.deletedSignal().connect( G::Slot::slot(*this,&GVerifiers::NetworkVerifier::clientDeleted) ) ;
}

GVerifiers::NetworkVerifier::~NetworkVerifier()
{
	m_client_ptr.eventSignal().disconnect() ;
	m_client_ptr.deletedSignal().disconnect() ;
}

void GVerifiers::NetworkVerifier::verify( Command command , const std::string & mail_to_address ,
	const GSmtp::Verifier::Info & info )
{
	m_command = command ;
	if( m_client_ptr.get() == nullptr )
	{
		unsigned int idle_timeout = 0U ;
		m_client_ptr.reset( std::make_unique<GSmtp::RequestClient>(
			GNet::ExceptionSink(m_client_ptr,m_es.esrc()),
			"verify" , "" ,
			m_location , m_connection_timeout , m_response_timeout ,
			idle_timeout ) ) ;
	}

	G_LOG( "GVerifiers::NetworkVerifier: verification request: ["
		<< G::Str::printable(mail_to_address) << "] (" << info.client_ip.displayString() << ")" ) ;

	G::StringArray args ;
	args.push_back( mail_to_address ) ;
	args.push_back( info.mail_from_parameter ) ;
	args.push_back( info.client_ip.displayString() ) ;
	args.push_back( info.domain ) ;
	args.push_back( G::Str::lower(info.auth_mechanism) ) ;
	args.push_back( info.auth_extra ) ;

	m_to_address = mail_to_address ;
	m_client_ptr->request( G::Str::join("|",args) ) ;
}

void GVerifiers::NetworkVerifier::clientDeleted( const std::string & reason )
{
	G_DEBUG( "GVerifiers::NetworkVerifier::clientDeleted: reason=[" << reason << "]" ) ;
	if( !reason.empty() )
	{
		std::string to_address = m_to_address ;
		m_to_address.erase() ;

		auto status = GSmtp::VerifierStatus::invalid( to_address , true , "cannot verify" , reason ) ;
		doneSignal().emit( m_command , status ) ;
	}
}

void GVerifiers::NetworkVerifier::clientEvent( const std::string & s1 , const std::string & s2 , const std::string & )
{
	G_DEBUG( "GVerifiers::NetworkVerifier::clientEvent: [" << s1 << "] [" << s2 << "]" ) ;
	if( s1 == "verify" )
	{
		G_LOG( "GVerifiers::NetworkVerifier: verification response: [" << G::Str::printable(s2) << "]" ) ;

		// parse the output from the remote verifier using pipe-delimited
		// fields based on the script-based verifier interface, but backwards
		//
		G::string_view s2_sv( s2 ) ;
		G::StringFieldView f( s2_sv , '|' ) ;
		std::size_t part_count = f.count() ;
		G::string_view part_0 = f() ;
		G::string_view part_1 = (++f)() ;
		G::string_view part_2 = (++f)() ;

		auto status = GSmtp::VerifierStatus::invalid( m_to_address ) ;
		if( part_count && part_0 == "100"_sv )
		{
			status.is_valid = false ;
			status.abort = true ;
		}
		else if( part_count >= 2U && part_0 == "1"_sv )
		{
			std::string address = G::sv_to_string( part_1 ) ;
			status = GSmtp::VerifierStatus::remote( m_to_address , address ) ;
		}
		else if( part_count >= 3U && part_0 == "0"_sv )
		{
			std::string mbox = G::sv_to_string( part_1 ) ;
			std::string full_name = G::sv_to_string( part_2 ) ;
			status = GSmtp::VerifierStatus::local( m_to_address , full_name , mbox ) ;
		}
		else if( part_count >= 2U && ( part_0 == "2"_sv || part_0 == "3"_sv ) )
		{
			bool temporary = part_0 == "3"_sv ;
			std::string response = G::sv_to_string( part_1 ) ;
			std::string reason ;
			if( part_count >= 3U ) reason = G::sv_to_string( part_2 ) ;
			status = GSmtp::VerifierStatus::invalid( m_to_address ,
				temporary , response , reason ) ;
		}

		doneSignal().emit( m_command , status ) ;
	}
}

G::Slot::Signal<GSmtp::Verifier::Command,const GSmtp::VerifierStatus&> & GVerifiers::NetworkVerifier::doneSignal()
{
	return m_done_signal ;
}

void GVerifiers::NetworkVerifier::cancel()
{
	m_to_address.erase() ;
	m_client_ptr.reset() ;
}

