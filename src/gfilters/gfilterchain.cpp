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
#include "gstringtoken.h"
#include "gstr.h"
#include "glog.h"
#include <utility>
#include <algorithm>

GFilters::FilterChain::FilterChain( GNet::ExceptionSink es , GSmtp::FilterFactoryBase & ff ,
	Filter::Type filter_type , const Filter::Config & filter_config ,
	const GSmtp::FilterFactoryBase::Spec & spec ,
	const std::string & log_prefix ) :
		m_filter_index(0U) ,
		m_filter(nullptr) ,
		m_running(false) ,
		m_message_id(GStore::MessageId::none())
{
	using Spec = GSmtp::FilterFactoryBase::Spec ;
	G_ASSERT( spec.first == "chain" ) ;
	for( G::StringToken t( spec.second , ","_sv ) ; t ; ++t )
	{
		std::string first = G::Str::head( t() , ":"_sv , false ) ;
		std::string second = G::Str::tail( t() , ":"_sv ) ;
		add( es , ff , filter_type , filter_config , Spec(first,second) , log_prefix ) ;
	}

	if( m_filters.empty() )
		add( es , ff , filter_type , filter_config , {"exit","0"} , log_prefix ) ;
}

void GFilters::FilterChain::add( GNet::ExceptionSink es , GSmtp::FilterFactoryBase & ff ,
	Filter::Type filter_type , const Filter::Config & filter_config ,
	const GSmtp::FilterFactoryBase::Spec & spec ,
	const std::string & log_prefix )
{
	m_filters.push_back( ff.newFilter( es , filter_type , filter_config , spec , log_prefix ) ) ;
	m_filter_id.append(m_filter_id.empty()?0U:1U,',').append( m_filters.back()->id() ) ;
}

GFilters::FilterChain::~FilterChain()
{
	if( m_filter )
		m_filter->doneSignal().disconnect() ;
}

std::string GFilters::FilterChain::id() const
{
	return m_filter_id ;
}

bool GFilters::FilterChain::quiet() const
{
    return std::all_of( m_filters.begin() , m_filters.end() ,
        [](const std::unique_ptr<Filter> & ptr){ return ptr->quiet() ; } ) ;
}

G::Slot::Signal<int> & GFilters::FilterChain::doneSignal() noexcept
{
	return m_done_signal ;
}

void GFilters::FilterChain::start( const GStore::MessageId & id )
{
	if( m_running )
	{
		m_filter->cancel() ;
		m_filter->doneSignal().disconnect() ;
	}
	m_running = true ;
	m_message_id = id ;
	m_filter_index = 0U ;
	m_filter = m_filters.at(0U).get() ;
	m_filter->doneSignal().connect( G::Slot::slot(*this,&FilterChain::onFilterDone) ) ;
	m_filter->start( m_message_id ) ;
}

void GFilters::FilterChain::onFilterDone( int ok_abandon_fail )
{
	m_filter->doneSignal().disconnect() ;
	if( ok_abandon_fail == 0 ) // ok
	{
		m_filter_index++ ;
		G_ASSERT( m_filter_index <= m_filters.size() ) ;
		if( m_filter_index >= m_filters.size() )
		{
			m_running = false ;
			m_done_signal.emit( 0 ) ;
		}
		else
		{
			m_filter = m_filters.at(m_filter_index).get() ;
			m_filter->doneSignal().connect( G::Slot::slot(*this,&FilterChain::onFilterDone) ) ;
			m_filter->start( m_message_id ) ;
		}
	}
	else // abandon/fail
	{
		G_DEBUG_IF( ((m_filter_index+1U)<m_filters.size()) , "GFilters::FilterChain::onFilterDone: some chained filters not run" ) ;
		m_running = false ;
		m_done_signal.emit( ok_abandon_fail ) ;
	}
}

void GFilters::FilterChain::cancel()
{
	if( m_running )
	{
		m_filter->cancel() ;
		m_filter->doneSignal().disconnect() ;
	}
	m_running = false ;
}

GSmtp::Filter::Result GFilters::FilterChain::result() const
{
	return m_filter->result() ;
}

std::string GFilters::FilterChain::response() const
{
	return m_filter->response() ;
}

std::string GFilters::FilterChain::reason() const
{
	return m_filter->reason() ;
}

bool GFilters::FilterChain::special() const
{
	return m_filter->special() ;
}

