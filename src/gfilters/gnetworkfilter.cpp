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
/// \file gnetworkfilter.cpp
///

#include "gdef.h"
#include "gnetworkfilter.h"
#include "gstr.h"
#include "gassert.h"
#include "glog.h"

GFilters::NetworkFilter::NetworkFilter( GNet::ExceptionSink es ,
	GStore::FileStore & file_store , Filter::Type , const Filter::Config & config ,
	const std::string & server ) :
		m_es(es) ,
		m_file_store(file_store) ,
		m_timer(*this,&NetworkFilter::onTimeout,m_es) ,
		m_done_signal(true) ,
		m_location(server) ,
		m_connection_timeout(config.timeout) ,
		m_response_timeout(config.timeout) ,
		m_result(Result::fail)
{
	m_client_ptr.eventSignal().connect( G::Slot::slot(*this,&GFilters::NetworkFilter::clientEvent) ) ;
}

GFilters::NetworkFilter::~NetworkFilter()
{
	m_client_ptr.eventSignal().disconnect() ;
}

std::string GFilters::NetworkFilter::id() const
{
	return m_location.displayString() ;
}

bool GFilters::NetworkFilter::quiet() const
{
	return false ;
}

void GFilters::NetworkFilter::start( const GStore::MessageId & message_id )
{
	m_text.clear() ;
	m_timer.cancelTimer() ;
	m_done_signal.reset() ;
	if( m_client_ptr.get() == nullptr || m_client_ptr->busy() )
	{
		unsigned int idle_timeout = 0U ;
		m_client_ptr.reset( std::make_unique<GSmtp::RequestClient>(
			GNet::ExceptionSink(*this,&m_client_ptr),
			"scanner" , "ok" ,
			m_location , m_connection_timeout , m_response_timeout ,
			idle_timeout ) ) ;
	}
	m_client_ptr->request( m_file_store.contentPath(message_id).str() ) ; // (no need to wait for connection)
}

void GFilters::NetworkFilter::onException( GNet::ExceptionSource * , std::exception & e , bool done )
{
	if( m_client_ptr.get() )
		m_client_ptr->doOnDelete( e.what() , done ) ;
	m_client_ptr.reset() ;

	sendResult( std::string("failed\t").append(e.what()) ) ;
}

void GFilters::NetworkFilter::clientEvent( const std::string & s1 , const std::string & s2 , const std::string & )
{
	if( s1 == "scanner" ) // ie. this is the response received by the RequestClient
	{
		sendResult( s2 ) ;
	}
}

void GFilters::NetworkFilter::sendResult( const std::string & reason )
{
	if( !m_text.has_value() )
	{
		m_text = reason ;
		m_timer.startTimer( 0 ) ;
		m_result = m_text.value().empty() ? Result::ok : Result::fail ;
	}
}

void GFilters::NetworkFilter::onTimeout()
{
	if( m_text.has_value() )
		m_done_signal.emit( static_cast<int>(m_result) ) ;
}

GSmtp::Filter::Result GFilters::NetworkFilter::result() const
{
	return m_result ;
}

bool GFilters::NetworkFilter::special() const
{
	return false ;
}

std::pair<std::string,int> GFilters::NetworkFilter::responsePair() const
{
	// "[<response-code> ]<response>[<tab><reason>]"
	std::string s = G::Str::printable( G::Str::head( m_text.value_or({}) , "\t" , false ) ) ;
	int n = 0 ;
	if( s.size() >= 3U &&
		( s[0] == '4' || s[0] == '5' ) &&
		( s[1] >= '0' && s[1] <= '9' ) &&
		( s[2] >= '0' && s[2] <= '9' ) &&
		( s.size() == 3U || s[3] == ' ' ) )
	{
		n = G::Str::toInt( s.substr(0U,3U) ) ;
		s.erase( 0U , s.size() == 3U ? 3U : 4U ) ;
	}
	return {s,n} ;
}

std::string GFilters::NetworkFilter::response() const
{
	return responsePair().first ;
}

int GFilters::NetworkFilter::responseCode() const
{
	return responsePair().second ;
}

std::string GFilters::NetworkFilter::reason() const
{
	return G::Str::printable( G::Str::tail( m_text.value_or({}) , "\t" , false ) ) ;
}

G::Slot::Signal<int> & GFilters::NetworkFilter::doneSignal() noexcept
{
	return m_done_signal ;
}

void GFilters::NetworkFilter::cancel()
{
	m_text.clear() ;
	m_timer.cancelTimer() ;
	m_done_signal.emitted( true ) ;
	m_client_ptr.reset() ;
}

