//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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

GVerifiers::NetworkVerifier::NetworkVerifier( GNet::EventState es , const GSmtp::Verifier::Config & config ,
	const std::string & server ) :
		m_es(es) ,
		m_config(config) ,
		m_location(server) ,
		m_connection_timeout(config.timeout) ,
		m_response_timeout(config.timeout)
{
	G_DEBUG( "GVerifiers::NetworkVerifier::ctor: " << server ) ;
	m_client_ptr.eventSignal().connect( G::Slot::slot(*this,&GVerifiers::NetworkVerifier::clientEvent) ) ;
}

GVerifiers::NetworkVerifier::~NetworkVerifier()
{
	m_client_ptr.eventSignal().disconnect() ;
	m_client_ptr.deletedSignal().disconnect() ;
}

void GVerifiers::NetworkVerifier::verify( const GSmtp::Verifier::Request & request )
{
	m_command = request.command ;
	if( m_client_ptr.get() == nullptr )
	{
		unsigned int idle_timeout = 0U ;
		m_client_ptr.reset( std::make_unique<GSmtp::RequestClient>(
			m_es.eh(this) ,
			"verify" , "" ,
			m_location , m_connection_timeout , m_response_timeout ,
			idle_timeout ) ) ;
	}

	G_LOG( "GVerifiers::NetworkVerifier: verification request: ["
		<< G::Str::printable(request.address) << "] (" << request.client_ip.displayString() << ")" ) ;

	G::StringArray args ;
	args.push_back( request.address ) ;
	args.push_back( request.from_address ) ;
	args.push_back( request.client_ip.displayString() ) ;
	args.push_back( m_config.domain ) ;
	args.push_back( G::Str::lower(request.auth_mechanism) ) ;
	args.push_back( request.auth_extra ) ;

	m_to_address = request.address ;
	m_client_ptr->request( G::Str::join("|",args) ) ;
}

void GVerifiers::NetworkVerifier::onException( GNet::ExceptionSource * , std::exception & e , bool done )
{
	bool was_busy = m_client_ptr.get() && m_client_ptr->busy() ;
	if( m_client_ptr.get() )
		m_client_ptr->doOnDelete( e.what() , done ) ;
	m_client_ptr.reset() ;

	if( was_busy )
	{
		std::string to_address = m_to_address ;
		m_to_address.erase() ;
		auto status = GSmtp::VerifierStatus::invalid( to_address , true , "cannot verify" , "network verifier peer disconnected" ) ;
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
		std::string_view s2_sv( s2 ) ;
		G::StringFieldView f( s2_sv , '|' ) ;
		std::size_t part_count = f.count() ;
		std::string_view part_0 = f() ;
		std::string_view part_1 = (++f)() ;
		std::string_view part_2 = (++f)() ;

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

