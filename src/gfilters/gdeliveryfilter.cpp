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
/// \file gdeliveryfilter.cpp
///

#include "gdef.h"
#include "gdeliveryfilter.h"
#include "gfiledelivery.h"
#include "glog.h"

GFilters::DeliveryFilter::DeliveryFilter( GNet::ExceptionSink es , GStore::FileStore & store ,
	Filter::Type filter_type , const Filter::Config & filter_config , const std::string & spec ) :
		m_store(store) ,
		m_filter_type(filter_type) ,
		m_filter_config(filter_config) ,
		m_spec(spec) ,
		m_timer(*this,&DeliveryFilter::onTimeout,es) ,
		m_result(Result::fail)
{
}

GFilters::DeliveryFilter::~DeliveryFilter()
= default ;

std::string GFilters::DeliveryFilter::id() const
{
	return "deliver" ;
}

bool GFilters::DeliveryFilter::simple() const
{
	return false ;
}

void GFilters::DeliveryFilter::start( const GStore::MessageId & message_id )
{
	if( m_filter_type != Filter::Type::server )
	{
		G_WARNING( "GFilters::DeliveryFilter::start: invalid use of the delivery filter" ) ;
		return ;
	}

	GStore::FileDelivery delivery_imp( m_store , m_filter_config.domain ) ;
	GStore::MessageDelivery & delivery = delivery_imp ;
	delivery.deliver( message_id ) ;

	m_result = Result::abandon ; // (original message deleted)
	m_timer.startTimer( 0U ) ;
}

G::Slot::Signal<int> & GFilters::DeliveryFilter::doneSignal() noexcept
{
	return m_done_signal ;
}

void GFilters::DeliveryFilter::cancel()
{
	m_timer.cancelTimer() ;
}

GSmtp::Filter::Result GFilters::DeliveryFilter::result() const
{
	return m_result ;
}

std::string GFilters::DeliveryFilter::response() const
{
	return {} ;
}

std::string GFilters::DeliveryFilter::reason() const
{
	return {} ;
}

bool GFilters::DeliveryFilter::special() const
{
	return false ;
}

void GFilters::DeliveryFilter::onTimeout()
{
	m_done_signal.emit( static_cast<int>(m_result) ) ;
}

