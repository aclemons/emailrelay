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
/// \file gcopyfilter.cpp
///

#include "gdef.h"
#include "gcopyfilter.h"
#include "gfiledelivery.h"
#include "gdirectory.h"
#include "groot.h"
#include "glog.h"

GFilters::CopyFilter::CopyFilter( GNet::ExceptionSink es , GStore::FileStore & store ,
	Filter::Type filter_type , const Filter::Config & filter_config , const std::string & spec ) :
		SimpleFilterBase(es,filter_type,"copy:") ,
		m_store(store) ,
		m_filter_config(filter_config) ,
		m_spec(spec)
{
}

GFilters::CopyFilter::~CopyFilter()
= default ;

GSmtp::Filter::Result GFilters::CopyFilter::run( const GStore::MessageId & message_id ,
	bool & , GStore::FileStore::State e_state )
{
	G::Path content_path = m_store.contentPath( message_id ) ;
	G::Path envelope_path = m_store.envelopePath( message_id , e_state ) ;
	GStore::Envelope envelope = GStore::FileStore::readEnvelope( envelope_path ) ;

	G::DirectoryList list ;
	{
		G::Root claim_root ;
		list.readDirectories( m_store.directory() ) ;
	}

	G::StringArray copy_names ;
	G::StringArray ignore_names ;
	while( list.more() )
	{
		G::Path subdir = list.filePath() ;
		std::string name = subdir.basename() ;
		if( name.empty() || name.at(0U) == '.' || name == "postmaster" )
		{
			ignore_names.push_back( name ) ;
		}
		else
		{
			copy_names.push_back( name ) ;
			GStore::FileDelivery::deliverTo( m_store , subdir , envelope_path , content_path , /*hardlink=*/false ) ;
		}
	}

	if( copy_names.empty() )
		G_WARNING_ONCE( "GFilters::CopyFilter::start: copy filter: no sub-directories of [" << m_store.directory() << "] to copy in to" ) ;
	G_LOG( "GFilters::CopyFilter::start: copy filter: " << message_id.str() << " copied to [" << G::Str::join(",",copy_names) << "]"
		<< (ignore_names.empty()?"":" not [") << G::Str::join(",",ignore_names) << (ignore_names.empty()?"":"]") ) ;

	return Result::ok ;
}

