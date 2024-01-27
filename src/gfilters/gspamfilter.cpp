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
/// \file gspamfilter.cpp
///

#include "gdef.h"
#include "gspamfilter.h"
#include "gstr.h"
#include "glog.h"

GFilters::SpamFilter::SpamFilter( GNet::ExceptionSink es , GStore::FileStore & file_store ,
	Filter::Type , const Filter::Config & config , const std::string & server ,
	bool read_only , bool always_pass ) :
		m_es(es) ,
		m_done_timer(*this,&SpamFilter::onDoneTimeout,m_es) ,
		m_done_signal(true) ,
		m_file_store(file_store) ,
		m_location(server) ,
		m_read_only(read_only) ,
		m_always_pass(always_pass) ,
		m_connection_timeout(config.timeout) ,
		m_response_timeout(config.timeout)
{
	m_client_ptr.eventSignal().connect( G::Slot::slot(*this,&GFilters::SpamFilter::clientEvent) ) ;
	m_client_ptr.deletedSignal().connect( G::Slot::slot(*this,&GFilters::SpamFilter::clientDeleted) ) ;
}

GFilters::SpamFilter::~SpamFilter()
{
	m_client_ptr.eventSignal().disconnect() ;
	m_client_ptr.deletedSignal().disconnect() ;
}

std::string GFilters::SpamFilter::id() const
{
	return m_location.displayString() ;
}

bool GFilters::SpamFilter::quiet() const
{
	return false ;
}

void GFilters::SpamFilter::start( const GStore::MessageId & message_id )
{
	// the spam client can do more than one request, but it is simpler to start fresh
	m_client_ptr.reset( std::make_unique<GSmtp::SpamClient>( GNet::ExceptionSink(m_client_ptr,m_es.esrc()) ,
		m_location , m_read_only , m_connection_timeout , m_response_timeout ) ) ;

	m_done_signal.emitted( false ) ;
	m_text.erase() ;
	m_client_ptr->request( m_file_store.contentPath(message_id).str() ) ; // (no need to wait for connection)
}

void GFilters::SpamFilter::clientDeleted( const std::string & reason )
{
	if( !reason.empty() && !m_done_signal.emitted() )
	{
		G_WARNING( "GFilters::SpamFilter::clientDeleted: spamd interaction failed: " << reason ) ;
	}
	m_text = reason ;
	done() ;
}

void GFilters::SpamFilter::clientEvent( const std::string & s1 , const std::string & s2 , const std::string & )
{
	G_DEBUG( "GFilters::SpamFilter::clientEvent: [" << s1 << "] [" << s2 << "]" ) ;
	if( s1 == "spam" )
	{
		m_text = ( s2.empty() || m_always_pass ) ? std::string() : std::string("spam: ").append(G::Str::printable(s2)) ;
		done() ;
	}
	else if( s1 == "failed" )
	{
		m_text = G::Str::printable( s2 ) ;
		done() ;
	}
}

bool GFilters::SpamFilter::special() const
{
	return false ;
}

void GFilters::SpamFilter::done()
{
	m_done_timer.startTimer( 0U ) ;
}

void GFilters::SpamFilter::onDoneTimeout()
{
	m_result = m_text.empty() ? Result::ok : Result::fail ;
	m_done_signal.emit( static_cast<int>(m_result) ) ;
}

GSmtp::Filter::Result GFilters::SpamFilter::result() const
{
	return m_result ;
}

std::string GFilters::SpamFilter::response() const
{
	return m_text.empty() ? std::string() : std::string("rejected") ;
}

int GFilters::SpamFilter::responseCode() const
{
	return 0 ;
}

std::string GFilters::SpamFilter::reason() const
{
	return m_text ;
}

G::Slot::Signal<int> & GFilters::SpamFilter::doneSignal() noexcept
{
	return m_done_signal ;
}

void GFilters::SpamFilter::cancel()
{
	G_DEBUG( "GFilters::SpamFilter::cancel: cancelled" ) ;
	m_done_timer.cancelTimer() ;
	m_text.erase() ;
	if( m_client_ptr.get() != nullptr && m_client_ptr->busy() )
		m_client_ptr.reset() ;
}

