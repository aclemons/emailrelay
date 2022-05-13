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
/// \file gspamfilter.cpp
///

#include "gdef.h"
#include "gspamfilter.h"
#include "gstr.h"
#include "glog.h"

GSmtp::SpamFilter::SpamFilter( GNet::ExceptionSink es , FileStore & file_store ,
	const std::string & server ,
	bool read_only , bool always_pass , unsigned int connection_timeout ,
	unsigned int response_timeout ) :
		m_es(es) ,
		m_file_store(file_store) ,
		m_location(server) ,
		m_read_only(read_only) ,
		m_always_pass(always_pass) ,
		m_connection_timeout(connection_timeout) ,
		m_response_timeout(response_timeout)
{
	m_client_ptr.eventSignal().connect( G::Slot::slot(*this,&GSmtp::SpamFilter::clientEvent) ) ;
	m_client_ptr.deletedSignal().connect( G::Slot::slot(*this,&GSmtp::SpamFilter::clientDeleted) ) ;
}

GSmtp::SpamFilter::~SpamFilter()
{
	m_client_ptr.eventSignal().disconnect() ;
	m_client_ptr.deletedSignal().disconnect() ;
}

std::string GSmtp::SpamFilter::id() const
{
	return m_location.displayString() ;
}

bool GSmtp::SpamFilter::simple() const
{
	return false ;
}

void GSmtp::SpamFilter::start( const MessageId & message_id )
{
	// the spam client can do more than one request, but it is simpler to start fresh
	m_client_ptr.reset( std::make_unique<SpamClient>( GNet::ExceptionSink(m_client_ptr,m_es.esrc()) ,
		m_location , m_read_only , m_connection_timeout , m_response_timeout ) ) ;

	m_text.erase() ;
	m_client_ptr->request( m_file_store.contentPath(message_id).str() ) ; // (no need to wait for connection)
}

void GSmtp::SpamFilter::clientDeleted( const std::string & reason )
{
	if( !reason.empty() )
	{
		G_WARNING( "GSmtp::SpamFilter::clientDeleted: spamd interaction failed: " << reason ) ;
		m_text = reason ;
		emit( false ) ;
	}
}

void GSmtp::SpamFilter::clientEvent( const std::string & s1 , const std::string & s2 , const std::string & )
{
	G_DEBUG( "GSmtp::SpamFilter::clientEvent: [" << s1 << "] [" << s2 << "]" ) ;
	if( s1 == "spam" )
	{
		m_text = ( s2.empty() || m_always_pass ) ? std::string() : ("spam: "+G::Str::printable(s2)) ;
		emit( m_text.empty() ) ;
	}
	else if( s1 == "failed" )
	{
		m_text = G::Str::printable(s2) ;
		emit( m_text.empty() ) ;
	}
}

void GSmtp::SpamFilter::emit( bool ok )
{
	m_done_signal.emit( ok ? 0 : 2 ) ;
}

bool GSmtp::SpamFilter::special() const
{
	return false ;
}

std::string GSmtp::SpamFilter::response() const
{
	return m_text.empty() ? std::string() : std::string("rejected") ;
}

std::string GSmtp::SpamFilter::reason() const
{
	return m_text ;
}

G::Slot::Signal<int> & GSmtp::SpamFilter::doneSignal()
{
	return m_done_signal ;
}

void GSmtp::SpamFilter::cancel()
{
	G_DEBUG( "GSmtp::SpamFilter::cancel: cancelled" ) ;
	m_text.erase() ;
	if( m_client_ptr.get() != nullptr && m_client_ptr->busy() )
		m_client_ptr.reset() ;
}

bool GSmtp::SpamFilter::abandoned() const
{
	return false ;
}

