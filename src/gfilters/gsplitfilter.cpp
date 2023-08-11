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
/// \file gsplitfilter.cpp
///

#include "gdef.h"
#include "gsplitfilter.h"
#include "gstoredfile.h"
#include "gprocess.h"
#include "gstringview.h"
#include "gstringtoken.h"
#include "gexception.h"
#include "gscope.h"
#include "gassert.h"
#include "glog.h"
#include <algorithm>
#include <iterator>

GFilters::SplitFilter::SplitFilter( GNet::ExceptionSink es , GStore::FileStore & store ,
	Filter::Type filter_type , const Filter::Config & filter_config , const std::string & spec ) :
		SimpleFilterBase(es,filter_type,"split:") ,
		m_store(store) ,
		m_filter_config(filter_config)
{
	G::string_view spec_sv = spec ;
	for( G::StringTokenView t(spec_sv,";",1U) ; t ; ++t )
	{
		if( t() == "raw"_sv ) m_raw = true ; // case-sensitive domain names
		if( G::Str::isNumeric(t()) ) m_port = G::sv_to_string(t()) ;
	}
}

GSmtp::Filter::Result GFilters::SplitFilter::run( const GStore::MessageId & message_id ,
	bool & , GStore::FileStore::State e_state )
{
	G::Path content_path = m_store.contentPath( message_id ) ;
	G::Path envelope_path = m_store.envelopePath( message_id , e_state ) ;

	GStore::Envelope envelope = m_store.readEnvelope( envelope_path ) ;

	// group-by domain
	G::StringArray domains ;
	std::transform( envelope.to_remote.begin() , envelope.to_remote.end() ,
		std::back_inserter(domains) , [](std::string &to_){return G::Str::tail(to_,"@");} ) ;
	bool raw = m_raw ;
	std::for_each( domains.begin() , domains.end() , [raw](std::string &to_){normalise(to_,raw);} ) ;
	std::sort( domains.begin() , domains.end() ) ;
	domains.erase( std::unique(domains.begin(),domains.end()) , domains.end() ) ;
	if( domains.empty() )
	{
		G_LOG( "GFilters::SplitFilter::start: " << prefix() << ": no remote domains: nothing to do" ) ;
		return Result::ok ;
	}

	// assign a message-id per domain
	G::StringArray ids ;
	ids.reserve( domains.size() ) ;
	ids.push_back( message_id.str() ) ;
	for( std::size_t i = 1U ; i < domains.size() ; i++ )
		ids.push_back( m_store.newId().str() ) ;

	// prepare extra headers giving the message ids of the split group
	std::stringstream extra_headers ;
	if( ids.size() > 1U )
	{
		extra_headers << m_store.x() << "SplitGroupCount: " << ids.size() << "\n" ;
		for( const auto & id : ids )
			extra_headers << m_store.x() << "SplitGroup: " << id << "\n" ;
	}

	// create new messages for each domain
	for( std::size_t i = 1U ; i < domains.size() ; i++ )
	{
		std::string domain = domains[i] ;
		GStore::MessageId new_id( ids[i] ) ;
		G::StringArray recipients = matching( envelope.to_remote , domain ) ;
		G_ASSERT( !recipients.empty() ) ;

		G::Path new_content_path = m_store.contentPath( new_id ) ;
		G::Path new_envelope_path = m_store.envelopePath( new_id ) ;

		G_LOG( "GFilters::SplitFilter::start: " << prefix() << " creating "
			<< "[" << new_id.str() << "]: forward-to=[" << domain << "]" ) ;

		GStore::Envelope new_envelope = envelope ;
		new_envelope.to_local.clear() ;
		new_envelope.to_remote = recipients ;
		new_envelope.forward_to = forwardTo( recipients.at(0U) ) ;

		if( !FileOp::hardlink( content_path , new_content_path ) )
			throw G::Exception( "split: cannot copy content file" ,
				new_content_path.str() , G::Process::strerror(FileOp::errno_()) ) ;
		G::ScopeExit clean_up_content( [new_content_path](){FileOp::remove(new_content_path);} ) ;

		std::ofstream new_envelope_stream ;
		FileOp::openOut( new_envelope_stream , new_envelope_path ) ;
		GStore::Envelope::write( new_envelope_stream , new_envelope ) ;
		GStore::Envelope::copyExtra( extra_headers , new_envelope_stream ) ;
		extra_headers.clear() ;
		extra_headers.seekg( 0 ) ;

		new_envelope_stream.close() ;
		if( !new_envelope_stream )
			throw G::Exception( "split: cannot create envelope file" ,
				new_envelope_path.str() , G::Process::strerror(FileOp::errno_()) ) ;

		clean_up_content.release() ;
	}

	// update the original message
	G_LOG( "GFilters::SplitFilter::start: " << prefix() << " updating "
		<< "[" << message_id.str() << "]: forward-to=[" << domains[0] << "]" ) ;
	G_ASSERT( !domains.empty() ) ;
	G::StringArray recipients = matching( envelope.to_remote , domains.at(0) ) ;
	G_ASSERT( !recipients.empty() ) ;
	std::string forward_to = forwardTo( recipients.at(0U) ) ;
	GStore::StoredFile msg( m_store , message_id , GStore::StoredFile::State::New ) ;
	msg.editEnvelope( [forward_to,&recipients](GStore::Envelope &env_){
			env_.to_remote = recipients ;
			env_.forward_to = forward_to ;
		} , &extra_headers ) ;

	return Result::ok ;
}

G::StringArray GFilters::SplitFilter::matching( const G::StringArray & recipients , const std::string & domain ) const
{
	G::StringArray result ;
	bool raw = m_raw ;
	std::copy_if( recipients.begin() , recipients.end() , std::back_inserter(result) ,
		[domain,raw](const std::string &to_){return match(G::Str::tail(to_,"@"),domain,raw) ;} ) ;
	return result ;
}

bool GFilters::SplitFilter::match( const std::string & a , const std::string & b , bool raw )
{
	return raw ? a == b : G::Str::imatch( a , b ) ;
}

void GFilters::SplitFilter::normalise( std::string & domain , bool raw )
{
	if( !raw )
		G::Str::toLower( domain ) ;
}

std::string GFilters::SplitFilter::forwardTo( const std::string & recipient ) const
{
	// user@example.com -> example.com:25
	return G::Str::tail(recipient,"@").append(m_port.empty()?0U:1U,':').append(m_port) ;
}
