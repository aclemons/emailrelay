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
		m_es(es) ,
		m_store(store) ,
		m_filter_type(filter_type) ,
		m_filter_config(filter_config) ,
		m_spec(spec) ,
		m_id("mx:") ,
		m_timer(*this,&MxFilter::onTimeout,es)
{
	if( !MxLookup::enabled() )
		throw G::Exception( "mx: not enabled at build time" ) ;
}

GFilters::MxFilter::~MxFilter()
{
	if( m_lookup )
		m_lookup->doneSignal().disconnect() ;
}

void GFilters::MxFilter::start( const GStore::MessageId & message_id )
{
	G::Path envelope_path = m_store.envelopePath( message_id , storestate() ) ;
	GStore::Envelope envelope = GStore::FileStore::readEnvelope( envelope_path ) ;
	unsigned int port = parseForwardToPort( envelope.forward_to ) ;
	std::string domain = parseForwardToDomain( envelope.forward_to ) ;
	if( domain.empty() )
	{
		m_timer.startTimer( 0U ) ;
		m_result = Result::ok ;
	}
	else
	{
		G_LOG( "GFilters::MxFilter::start: " << prefix() << " looking up [" << domain << "]" ) ;

		if( m_lookup ) m_lookup->doneSignal().disconnect() ;
		m_lookup = std::make_unique<MxLookup>( m_es , mxconfig(m_spec) , mxnameservers(m_spec) ) ;
		m_lookup->doneSignal().connect( G::Slot::slot(*this,&MxFilter::lookupDone) ) ;

		m_lookup->start( message_id , domain , port ) ;
		if( m_filter_config.timeout )
			m_timer.startTimer( m_filter_config.timeout ) ;
		else
			m_timer.cancelTimer() ;
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
	return { m_result == Result::fail ? "failed" : "" } ;
}

int GFilters::MxFilter::responseCode() const
{
	return 0 ;
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
	G_DEBUG( "GFilters::MxFilter::onTimeout: response=[" << response() << "] special=" << m_special ) ;
	m_done_signal.emit( static_cast<int>(m_result) ) ;
}

void GFilters::MxFilter::lookupDone( GStore::MessageId message_id , std::string address , std::string error )
{
	G_ASSERT( address.empty() == !error.empty() ) ;

	// allow a special IP address to mean no forward-to-address
	if( G::Str::headMatch(address,"0.0.0.0:") && GNet::Address::validString(address) )
		address.clear() ;

	G_LOG( "GFilters::MxFilter::start: " << prefix() << ": [" << message_id.str() << "]: "
		<< "setting forward-to-address [" << address << "]"
		<< (error.empty()?"":" (") << error << (error.empty()?"":")") ) ;

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

// TODO combine mxconfig() and mxnameservers() to better report errors (fr #36)

GFilters::MxLookup::Config GFilters::MxFilter::mxconfig( const std::string & spec )
{
	MxLookup::Config config ;
	G::string_view spec_sv = spec ;
	for( G::StringTokenView t(spec_sv,";",1U) ; t ; ++t )
	{
		if( t().find("nst=") == 0U && t().size() > 4U )
			config.ns_timeout = G::TimeInterval( G::Str::toUInt(t().substr(4U),"1") ) ;
		else if( t().find("rt=") == 0U && t().size() > 3U )
			config.restart_timeout = G::TimeInterval( G::Str::toUInt(t().substr(3U),"15") ) ;
	}
	return config ;
}

std::vector<GNet::Address> GFilters::MxFilter::mxnameservers( const std::string & spec )
{
	std::vector<GNet::Address> result ;
	G::string_view spec_sv = spec ;
	for( G::StringTokenView t(spec_sv,";",1U) ; t ; ++t )
	{
		std::string s = G::sv_to_string( t() ) ;
		if( GNet::Address::validString( s ) )
			result.push_back( GNet::Address::parse( s ) ) ;
		else if( GNet::Address::validStrings( s , "53" ) )
			result.push_back( GNet::Address::parse( s , "53" ) ) ;
	}
	return result.empty() ? GNet::nameservers(53U) : result ;
}

std::string GFilters::MxFilter::parseForwardToDomain( const std::string & forward_to )
{
	return parseForwardTo(forward_to).first ;
}

unsigned int GFilters::MxFilter::parseForwardToPort( const std::string & forward_to )
{
	return parseForwardTo(forward_to).second ;
}

std::pair<std::string,unsigned int> GFilters::MxFilter::parseForwardTo( const std::string & forward_to )
{
	// "example.com:<port>"
	// "example.com"
	// "user@example.com:<port>"
	// "user@example.com"
	//
	auto no_user = G::Str::tailView( forward_to , "@" , false ) ;
	std::size_t pos = no_user.rfind( ':' ) ;
	auto head = G::Str::headView( no_user , pos , no_user ) ;
	auto tail = G::Str::tailView( no_user , pos , {} ) ;
	bool with_port = !tail.empty() && G::Str::isNumeric( tail ) ;
	auto first = with_port ? head : no_user ;
	auto second = with_port ? G::Str::toUInt(tail) : 0U ;
	return { G::sv_to_string(first) , second } ;
}

std::string GFilters::MxFilter::prefix() const
{
	return G::sv_to_string(strtype(m_filter_type)).append(" [").append(id()).append(1U,']') ;
}

