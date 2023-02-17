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
/// \file gmxfilter.cpp
///

#include "gdef.h"
#include "gmxfilter.h"
#include "gstoredfile.h"
#include "gprocess.h"
#include "gaddress.h"
#include "gnameservers.h"
#include "gexception.h"
#include "gscope.h"
#include "gstringtoken.h"
#include "glog.h"

GFilters::MxFilter::MxFilter( GNet::ExceptionSink es , GStore::FileStore & store ,
	Filter::Type filter_type , const Filter::Config & filter_config , const std::string & spec ) :
		m_store(store) ,
		m_filter_type(filter_type) ,
		m_filter_config(filter_config) ,
		m_spec(spec) ,
		m_id("mx:") ,
		m_result(Result::fail) ,
		m_special(false) ,
		m_timer(*this,&MxFilter::onTimeout,es)
{
	if( filter_type != Filter::Type::client )
	{
		if( !MxLookup::enabled() )
			throw G::Exception( "mx: dns mx routing not enabled at build time" ) ;
		m_lookup = std::make_unique<MxLookup>( es , mxconfig(m_spec) , mxnameservers(m_spec) ) ;
		m_lookup->doneSignal().connect( G::Slot::slot(*this,&MxFilter::lookupDone) ) ;
	}
}

GFilters::MxFilter::~MxFilter()
{
	if( m_lookup )
		m_lookup->doneSignal().disconnect() ;
}

void GFilters::MxFilter::start( const GStore::MessageId & message_id )
{
	m_timer.cancelTimer() ;
	if( m_filter_type == Filter::Type::client )
	{
		m_timer.startTimer( 0U ) ;
		m_result = Result::ok ;
	}
	else
	{
		G::Path envelope_path = m_store.envelopePath( message_id , storestate() ) ;
		GStore::Envelope envelope = m_store.readEnvelope( envelope_path ) ;
		std::string domain = G::Str::tail( envelope.forward_to , "@" , false ) ;
		if( domain.empty() )
		{
			m_timer.startTimer( 0U ) ;
			m_result = Result::ok ;
		}
		else
		{
			G_LOG( "GFilters::MxFilter::start: mx: " << message_id.str() << ": looking up [" << domain << "]" ) ;
			m_lookup->start( message_id , domain ) ;
			if( m_filter_config.timeout )
				m_timer.startTimer( m_filter_config.timeout ) ;
		}
	}
}

std::string GFilters::MxFilter::id() const
{
	return m_id ;
}

bool GFilters::MxFilter::quiet() const
{
	return false ;
}

bool GFilters::MxFilter::special() const
{
	return m_special ;
}

GSmtp::Filter::Result GFilters::MxFilter::result() const
{
	return m_result ;
}

std::string GFilters::MxFilter::response() const
{
	return std::string( m_result == Result::fail ? "failed" : "" ) ;
}

std::string GFilters::MxFilter::reason() const
{
	return response() ;
}

G::Slot::Signal<int> & GFilters::MxFilter::doneSignal() noexcept
{
	return m_done_signal ;
}

void GFilters::MxFilter::cancel()
{
	if( m_lookup )
		m_lookup->cancel() ;
}

void GFilters::MxFilter::onTimeout()
{
	m_done_signal.emit( static_cast<int>(m_result) ) ;
}

void GFilters::MxFilter::lookupDone( GStore::MessageId message_id , std::string address , std::string error )
{
	G_ASSERT( address.empty() == !error.empty() ) ;

	// allow a special IP address to mean no forward-to-address
	if( G::Str::headMatch(address,"0.0.0.0:") && GNet::Address::validString(address) )
		address.clear() ;

	G_LOG( "GFilters::MxFilter::start: mx: " << message_id.str() << ": "
		<< "setting forward-to-address [" << address << "]"
		<< (error.empty()?"":": ") << error ) ;

	// update the envelope forward-to-address
	GStore::StoredFile msg( m_store , message_id , storestate() ) ;
	msg.noUnlock() ;
	msg.editEnvelope( [address](GStore::Envelope &env_){env_.forward_to_address=address;} ) ;

	m_result = error.empty() ? Result::ok : Result::fail ;
	m_timer.startTimer( 0U ) ;
}

GStore::FileStore::State GFilters::MxFilter::storestate() const
{
	return m_filter_type == GSmtp::Filter::Type::server ?
		GStore::FileStore::State::New :
		GStore::FileStore::State::Locked ;
}

GFilters::MxLookup::Config GFilters::MxFilter::mxconfig( const std::string & spec )
{
	MxLookup::Config result ;
	G::string_view spec_sv = spec ;
	for( G::StringTokenView t(spec_sv,";",1U) ; t ; ++t )
	{
		if( G::Str::isUInt(t()) )
			result.port = G::Str::toUInt( t() ) ;
	}
	return result ;
}

std::vector<GNet::Address> GFilters::MxFilter::mxnameservers( const std::string & spec )
{
	std::vector<GNet::Address> result ;
	G::string_view spec_sv = spec ;
	for( G::StringTokenView t(spec_sv,";",1U) ; t ; ++t )
	{
		if( GNet::Address::validString( G::sv_to_string(t()) ) )
			result.push_back( GNet::Address::parse( G::sv_to_string(t()) ) ) ;
	}
	return result.empty() ? GNet::nameservers(53U) : result ;
}

