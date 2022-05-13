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
/// \file gfilterchain.cpp
///

#include "gdef.h"
#include "gfilterchain.h"
#include "gslot.h"
#include "gstr.h"
#include <utility>
#include <algorithm>

GSmtp::FilterChain::FilterChain( GNet::ExceptionSink es , FileStore & file_store , FilterFactory & ff ,
	bool server_side , const std::string & spec , unsigned int timeout ) :
		m_file_store(file_store) ,
		m_server_side(server_side) ,
		m_filter_index(0U) ,
		m_filter(nullptr) ,
		m_running(false) ,
		m_message_id(MessageId::none())
{
	G::StringArray sub_specs = G::Str::splitIntoTokens( spec , {",",1U} ) ;
	if( sub_specs.empty() )
		sub_specs.push_back( "exit:0" ) ;
	for( const auto & sub_spec : sub_specs )
	{
		std::unique_ptr<Filter> filter = ff.newFilter( es , server_side , sub_spec , timeout ) ;
		m_filters.emplace_back( std::move(filter) ) ;
		m_filter_id = std::string(m_filter_id.empty()?1U:0U,',') + m_filters.back()->id() ;
	}
	m_filter_index = 0 ;
	m_filter = m_filters.at(0U).get() ;
	m_filter->doneSignal().connect( G::Slot::slot(*this,&FilterChain::onFilterDone) ) ;
}

GSmtp::FilterChain::~FilterChain()
{
	m_filter->doneSignal().disconnect() ;
}

std::string GSmtp::FilterChain::id() const
{
	return m_filter_id ;
}

bool GSmtp::FilterChain::simple() const
{
	return std::all_of( m_filters.begin() , m_filters.end() ,
		[](const std::unique_ptr<Filter> & ptr){ return ptr->simple() ; } ) ;
}

G::Slot::Signal<int> & GSmtp::FilterChain::doneSignal()
{
	return m_done_signal ;
}

void GSmtp::FilterChain::start( const MessageId & id )
{
	if( m_running )
		m_filter->cancel() ;
	m_running = true ;
	m_message_id = id ;
	m_filter->start( m_message_id ) ;
}

void GSmtp::FilterChain::onFilterDone( int ok_abandon_fail )
{
	if( ok_abandon_fail == 0 )
	{
		m_filter_index++ ;
		if( m_filter_index == m_filters.size() )
		{
			m_running = false ;
			m_done_signal.emit( 0 ) ;
		}
		else
		{
			m_filter->doneSignal().disconnect() ;
			m_filter = m_filters.at(m_filter_index).get() ;
			m_filter->doneSignal().connect( G::Slot::slot(*this,&FilterChain::onFilterDone) ) ;
			m_filter->start( m_message_id ) ;
		}
	}
	else
	{
		m_running = false ;
		m_done_signal.emit( ok_abandon_fail ) ;
	}
}

void GSmtp::FilterChain::cancel()
{
	if( m_running )
		m_filter->cancel() ;
	m_running = false ;
}

bool GSmtp::FilterChain::abandoned() const
{
	return m_filter->abandoned() ;
}

std::string GSmtp::FilterChain::response() const
{
	return m_filter->response() ;
}

std::string GSmtp::FilterChain::reason() const
{
	return m_filter->reason() ;
}

bool GSmtp::FilterChain::special() const
{
	return m_filter->special() ;
}

