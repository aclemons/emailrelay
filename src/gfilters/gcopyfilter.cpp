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
/// \file gcopyfilter.cpp
///

#include "gdef.h"
#include "gcopyfilter.h"
#include "gfiledelivery.h"
#include "gdirectory.h"
#include "gstringtoken.h"
#include "groot.h"
#include "gfile.h"
#include "glog.h"

GFilters::CopyFilter::CopyFilter( GNet::ExceptionSink es , GStore::FileStore & store ,
	Filter::Type filter_type , const Filter::Config & filter_config , const std::string & spec ) :
		SimpleFilterBase(es,filter_type,"copy:") ,
		m_store(store) ,
		m_filter_config(filter_config) ,
		m_spec(spec)
{
	G::string_view spec_sv = spec ;
	for( G::StringTokenView t( spec_sv , ";" , 1U ) ; t ; ++t )
	{
		if( t() == "p"_sv || t() == "pop"_sv ) m_pop_by_name = true ;
		if( t() == "h"_sv || t() == "hardlink"_sv ) m_hardlink = true ;
		if( t() == "n"_sv || t() == "nodelete"_sv || t() == "no_delete"_sv ) m_no_delete = true ;
	}
}

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
			GStore::FileDelivery::deliverTo( m_store , "copy" ,
				subdir , envelope_path , content_path ,
				m_hardlink , m_pop_by_name ) ;
		}
	}

	if( copy_names.empty() )
	{
		G_WARNING_ONCE( "GFilters::CopyFilter::start: copy filter: "
			"no sub-directories of [" << m_store.directory() << "] to copy in to" ) ;
		return Result::ok ;
	}
	else
	{
		G_LOG( "GFilters::CopyFilter::start: " << prefix() << ": "
			<< message_id.str() << " copied to [" << G::Str::join(",",copy_names) << "]"
			<< (ignore_names.empty()?"":" not [") << G::Str::join(",",ignore_names)
			<< (ignore_names.empty()?"":"]") ) ;

		if( m_no_delete )
		{
			return Result::ok ;
		}
		else
		{
			G::Root claim_root ;
			G::File::remove( envelope_path ) ;
			if( !m_pop_by_name )
				G::File::remove( content_path ) ;
			return Result::abandon ;
		}
	}
}

