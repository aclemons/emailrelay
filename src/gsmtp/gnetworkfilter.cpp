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
/// \file gnetworkfilter.cpp
///

#include "gdef.h"
#include "gnetworkfilter.h"
#include "gstr.h"
#include "glog.h"

GSmtp::NetworkFilter::NetworkFilter( GNet::ExceptionSink es , FileStore & file_store ,
	const std::string & server ,
	unsigned int connection_timeout , unsigned int response_timeout ) :
		m_es(es) ,
		m_file_store(file_store) ,
		m_location(server) ,
		m_connection_timeout(connection_timeout) ,
		m_response_timeout(response_timeout)
{
	m_client_ptr.eventSignal().connect( G::Slot::slot(*this,&GSmtp::NetworkFilter::clientEvent) ) ;
	m_client_ptr.deletedSignal().connect( G::Slot::slot(*this,&GSmtp::NetworkFilter::clientDeleted) ) ;
}

GSmtp::NetworkFilter::~NetworkFilter()
{
	m_client_ptr.eventSignal().disconnect() ;
	m_client_ptr.deletedSignal().disconnect() ;
}

std::string GSmtp::NetworkFilter::id() const
{
	return m_location.displayString() ;
}

bool GSmtp::NetworkFilter::simple() const
{
	return false ;
}

void GSmtp::NetworkFilter::start( const MessageId & message_id )
{
	m_text.erase() ;
	if( m_client_ptr.get() == nullptr )
	{
		m_client_ptr.reset( std::make_unique<RequestClient>(
			GNet::ExceptionSink(m_client_ptr,m_es.esrc()),
			"scanner" , "ok" ,
			m_location , m_connection_timeout , m_response_timeout ) ) ;
	}
	m_client_ptr->request( m_file_store.contentPath(message_id).str() ) ; // (no need to wait for connection)
}

void GSmtp::NetworkFilter::clientDeleted( const std::string & reason )
{
	if( !reason.empty() )
	{
		m_text = "failed" "\t" + reason ;
		m_done_signal.emit( 2 ) ;
	}
}

void GSmtp::NetworkFilter::clientEvent( const std::string & s1 , const std::string & s2 , const std::string & )
{
	if( s1 == "scanner" ) // ie. this is the response received by the RequestClient
	{
		m_text = s2 ;
		m_done_signal.emit( m_text.empty() ? 0 : 2 ) ;
	}
}

bool GSmtp::NetworkFilter::special() const
{
	return false ;
}

std::string GSmtp::NetworkFilter::response() const
{
	// allow "<response><tab><reason>"
	return G::Str::printable( G::Str::head( m_text , "\t" , false ) ) ;
}

std::string GSmtp::NetworkFilter::reason() const
{
	return G::Str::printable( G::Str::tail( m_text , "\t" , false ) ) ;
}

G::Slot::Signal<int> & GSmtp::NetworkFilter::doneSignal()
{
	return m_done_signal ;
}

void GSmtp::NetworkFilter::cancel()
{
	m_client_ptr.reset() ;
	m_text.erase() ;
}

bool GSmtp::NetworkFilter::abandoned() const
{
	return false ;
}
