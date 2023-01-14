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
#include "grange.h"
#include "glog.h"

GFilters::DeliveryFilter::DeliveryFilter( GNet::ExceptionSink es , GStore::FileStore & store ,
	Filter::Type filter_type , const Filter::Config & filter_config , const std::string & spec ) :
		SimpleFilterBase(es,filter_type,"deliver:") ,
		m_store(store) ,
		m_filter_type(filter_type) ,
		m_filter_config(filter_config) ,
		m_spec(spec)
{
}

GFilters::DeliveryFilter::~DeliveryFilter()
= default ;

GSmtp::Filter::Result GFilters::DeliveryFilter::run( const GStore::MessageId & message_id ,
	bool & , GStore::FileStore::State )
{
	if( m_filter_type != Filter::Type::server )
	{
		G_WARNING( "GFilters::DeliveryFilter::start: invalid use of the delivery filter" ) ;
		return Result::fail ;
	}

	GStore::FileDelivery delivery_imp( m_store , m_filter_config.domain , G::Range::range(m_spec) ) ;
	GStore::MessageDelivery & delivery = delivery_imp ;
	delivery.deliver( message_id ) ;

	return Result::abandon ; // (original message deleted)
}

