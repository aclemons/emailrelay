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
/// \file gsplitfilter.cpp
///

#include "gdef.h"
#include "gsplitfilter.h"
#include "gstoredfile.h"
#include "gprocess.h"
#include "gexception.h"
#include "gscope.h"
#include "glog.h"
#include <algorithm>
#include <iterator>

GFilters::SplitFilter::SplitFilter( GNet::ExceptionSink es , GStore::FileStore & store ,
	Filter::Type filter_type , const Filter::Config & filter_config , const std::string & spec ) :
		SimpleFilterBase(es,filter_type,"split:") ,
		m_store(store) ,
		m_filter_type(filter_type) ,
		m_filter_config(filter_config) ,
		m_spec(spec)
{
}

GFilters::SplitFilter::~SplitFilter()
= default ;

GSmtp::Filter::Result GFilters::SplitFilter::run( const GStore::MessageId & message_id ,
	bool & rescan_out , GStore::FileStore::State e_state )
{
	G::Path content_path = m_store.contentPath( message_id ) ;
	G::Path envelope_path = m_store.envelopePath( message_id , e_state ) ;

	GStore::Envelope envelope = m_store.readEnvelope( envelope_path ) ;

	// group-by domain
	G::StringArray domains ;
	std::transform( envelope.to_remote.begin() , envelope.to_remote.end() ,
		std::back_inserter(domains) , [](std::string &to_){return G::Str::tail(to_,"@");} ) ;
	std::sort( domains.begin() , domains.end() ) ;
	domains.erase( std::unique(domains.begin(),domains.end()) , domains.end() ) ;

	if( domains.size() >= 2U )
	{
		// assign a message-id per domain
		G::StringArray ids ;
		ids.reserve( domains.size() ) ;
		ids.push_back( message_id.str() ) ;
		for( std::size_t i = 1U ; i < domains.size() ; i++ )
			ids.push_back( m_store.newId().str() ) ;

		// create new messages for each domain
		for( std::size_t i = 1U ; i < domains.size() ; i++ )
		{
			std::string domain = domains[i] ;
			GStore::MessageId new_id( ids[i] ) ;
			G::StringArray recipients = match( envelope.to_remote , domain ) ;

			G::Path new_content_path = m_store.contentPath( new_id ) ;
			G::Path new_envelope_path = m_store.envelopePath( new_id ) ;

			G_LOG( "GFilters::SplitFilter::start: split: creating message "
				<< new_id.str() << ": setting forward-to [" << domain << "]" ) ;

			GStore::Envelope new_envelope = envelope ;
			new_envelope.to_local.clear() ;
			new_envelope.to_remote = recipients ;
			new_envelope.forward_to = domain ;

			if( !FileOp::hardlink( content_path , new_content_path ) )
				throw G::Exception( "split: cannot copy content file" ,
					new_content_path.str() , G::Process::strerror(FileOp::errno_()) ) ;
			G::ScopeExit clean_up_content( [new_content_path](){FileOp::remove(new_content_path);} ) ;

			std::ofstream new_envelope_stream ;
			FileOp::openOut( new_envelope_stream , new_envelope_path ) ;

			GStore::Envelope::write( new_envelope_stream , new_envelope ) ;
			for( const auto group_id : ids )
				new_envelope_stream << m_store.x() << "SplitGroup: " << group_id << "\r\n" ;
			new_envelope_stream.close() ;
			if( !new_envelope_stream )
				throw G::Exception( "split: cannot create envelope file" ,
					new_envelope_path.str() , G::Process::strerror(FileOp::errno_()) ) ;

			clean_up_content.release() ;
		}
	}

	// update the original message
	// TODO also add split-group info
	G_LOG( "GFilters::SplitFilter::start: split: updating message "
		<< message_id.str() << ": setting forward-to [" << domains[0] << "]" ) ;
	std::string domain = domains[0] ;
	G::StringArray recipients = match( envelope.to_remote , domain ) ;
	GStore::StoredFile msg( m_store , message_id , GStore::StoredFile::State::New ) ;
	msg.editEnvelope( [domain,&recipients](GStore::Envelope &env_){
			env_.to_remote = recipients ;
			env_.forward_to = domain ;
		} ) ;

	if( m_filter_type == Filter::Type::server && domains.size() >= 2U )
		rescan_out = true ;
	return Result::ok ;
}

G::StringArray GFilters::SplitFilter::match( const G::StringArray & recipients , const std::string & domain )
{
	G::StringArray result ;
	std::copy_if( recipients.begin() , recipients.end() , std::back_inserter(result) ,
		[domain](const std::string &to_){return G::Str::tail(to_,"@") == domain;} ) ;
	return result ;
}

