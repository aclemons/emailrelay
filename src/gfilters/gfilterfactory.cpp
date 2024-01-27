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
/// \file gfilterfactory.cpp
///

#include "gdef.h"
#include "gfilterfactory.h"
#include "gstringtoken.h"
#include "gfilterchain.h"
#include "gfilestore.h"
#include "gnullfilter.h"
#include "gfile.h"
#include "gnetworkfilter.h"
#include "gexecutablefilter.h"
#include "gspamfilter.h"
#include "gdeliveryfilter.h"
#include "gmessageidfilter.h"
#include "gcopyfilter.h"
#include "gmxfilter.h"
#include "gsplitfilter.h"
#include "gstr.h"
#include "grange.h"
#include "gexception.h"
#include <limits>

GFilters::FilterFactory::FilterFactory( GStore::FileStore & file_store ) :
	m_file_store(file_store)
{
}

GFilters::FilterFactory::Spec GFilters::FilterFactory::parse( const std::string & spec_in ,
	const G::Path & base_dir , const G::Path & app_dir , G::StringArray * warnings_p )
{
	std::string tail = G::Str::tail( spec_in , ":" ) ;
	Spec result ;
	if( spec_in.empty() )
	{
		result = Spec( "exit" , "0" ) ;
	}
	else if( spec_in.find(',') != std::string::npos )
	{
		result = Spec( "chain" , "" ) ;
		for( G::StringToken t( spec_in , ","_sv ) ; t ; ++t )
			result += parse( t() , base_dir , app_dir , warnings_p ) ; // one level of recursion
	}
	else if( G::Str::headMatch( spec_in ,"exit:" ) )
	{
		result = Spec( "exit" , tail ) ;
		checkNumber( result ) ;
	}
	else if( G::Str::headMatch( spec_in , "sleepms:" ) )
	{
		result = Spec( "sleep" , tail ) ;
		checkNumber( result ) ;
	}
	else if( G::Str::headMatch( spec_in , "net:" ) )
	{
		result = Spec( "net" , tail ) ;
		checkNet( result ) ;
	}
	else if( G::Str::headMatch( spec_in , "spam:" ) )
	{
		result = Spec( "spam" , tail ) ;
		checkNet( result ) ;
	}
	else if( G::Str::headMatch( spec_in , "spam-edit:" ) )
	{
		result = Spec( "spam-edit" , tail ) ;
		checkNet( result ) ;
	}
	else if( G::Str::headMatch( spec_in , "deliver:" ) )
	{
		result = Spec( "deliver" , tail ) ;
		checkRange( result ) ;
	}
	else if( G::Str::headMatch( spec_in , "copy:" ) )
	{
		result = Spec( "copy" , tail ) ;
	}
	else if( G::Str::headMatch( spec_in , "split:" ) )
	{
		result = Spec( "split" , tail ) ;
	}
	else if( G::Str::headMatch( spec_in , "mx:" ) )
	{
		result = Spec( "mx" , tail ) ;
	}
	else if( G::Str::headMatch( spec_in , "msgid:" ) )
	{
		result = Spec( "msgid" , tail ) ;
	}
	else if( G::Str::headMatch( spec_in , "file:" ) )
	{
		result = Spec( "file" , tail ) ;
		fixFile( result , base_dir , app_dir ) ;
		checkFile( result , warnings_p ) ;
	}
	else
	{
		result = Spec( "file" , spec_in ) ;
		fixFile( result , base_dir , app_dir ) ;
		checkFile( result , warnings_p ) ;
	}
	return result ;
}

std::unique_ptr<GSmtp::Filter> GFilters::FilterFactory::newFilter( GNet::ExceptionSink es ,
	GSmtp::Filter::Type filter_type , const GSmtp::Filter::Config & filter_config ,
	const FilterFactory::Spec & spec )
{
	if( spec.first == "chain" )
	{
		// (one level of recursion -- FilterChain::ctor calls newFilter())
		return std::make_unique<FilterChain>( es , *this , filter_type , filter_config , spec ) ;
	}
	else if( spec.first == "spam" )
	{
		// "spam:" is read-only, not-always-pass
		return std::make_unique<SpamFilter>( es , m_file_store , filter_type , filter_config , spec.second , true , false ) ;
	}
	else if( spec.first == "spam-edit" )
	{
		// "spam-edit:" is read-write, always-pass
		return std::make_unique<SpamFilter>( es , m_file_store , filter_type , filter_config , spec.second , false , true ) ;
	}
	else if( spec.first == "net" )
	{
		return std::make_unique<NetworkFilter>( es , m_file_store , filter_type , filter_config , spec.second ) ;
	}
	else if( spec.first == "exit" )
	{
		return std::make_unique<NullFilter>( es , m_file_store , filter_type , filter_config , G::Str::toUInt(spec.second) ) ;
	}
	else if( spec.first == "sleep" )
	{
		return std::make_unique<NullFilter>( es , m_file_store , filter_type , filter_config , G::TimeInterval(0U,G::Str::toUInt(spec.second)) ) ;
	}
	else if( spec.first == "file" )
	{
		return std::make_unique<ExecutableFilter>( es , m_file_store , filter_type , filter_config , spec.second ) ;
	}
	else if( spec.first == "deliver" )
	{
		return std::make_unique<DeliveryFilter>( es , m_file_store , filter_type , filter_config , spec.second ) ;
	}
	else if( spec.first == "copy" )
	{
		return std::make_unique<CopyFilter>( es , m_file_store , filter_type , filter_config , spec.second ) ;
	}
	else if( spec.first == "split" )
	{
		return std::make_unique<SplitFilter>( es , m_file_store , filter_type , filter_config , spec.second ) ;
	}
	else if( spec.first == "mx" )
	{
		return std::make_unique<MxFilter>( es , m_file_store , filter_type , filter_config , spec.second ) ;
	}
	else if( spec.first == "msgid" )
	{
		return std::make_unique<MessageIdFilter>( es , m_file_store , filter_type , filter_config , spec.second ) ;
	}
	else
	{
		throw G::Exception( "invalid filter" , spec.second ) ;
	}
}

void GFilters::FilterFactory::checkNumber( Spec & result )
{
	if( result.second.empty() )
	{
		result.first.clear() ;
		result.second = "numeric value missing" ;
	}
	else if( !G::Str::isUInt(result.second) )
	{
		result.first.clear() ;
		result.second = "invalid numeric value: " + G::Str::printable(result.second) ;
	}
	else if( G::Str::toUInt(result.second) >= (std::numeric_limits<unsigned>::max()/1000U) )
	{
		result.first.clear() ;
		result.second = "numeric value too big: " + G::Str::printable(result.second) ;
	}
}

void GFilters::FilterFactory::checkNet( Spec & result )
{
	try
	{
		GNet::Location::nosocks( result.second ) ;
	}
	catch( std::exception & e )
	{
		result.first.clear() ;
		result.second = e.what() ;
	}
}

void GFilters::FilterFactory::checkRange( Spec & result )
{
	try
	{
		G::Range::check( result.second ) ;
	}
	catch( std::exception & e )
	{
		result.first.clear() ;
		result.second = e.what() ;
	}
}

void GFilters::FilterFactory::fixFile( Spec & result ,
	const G::Path & base_dir , const G::Path & app_dir )
{
	if( result.second.find("@app") == 0U && !app_dir.empty() )
	{
		G::Str::replace( result.second , "@app" , app_dir.str() ) ;
	}
	else if( G::Path(result.second).isRelative() && !base_dir.empty() )
	{
		result.second = (base_dir+result.second).str() ;
	}
}

void GFilters::FilterFactory::checkFile( Spec & result , G::StringArray * warnings_p )
{
	if( result.second.empty() )
	{
		result.first.clear() ;
		result.second = "empty file path" ;
	}
	else if( warnings_p && !G::File::exists(result.second) )
	{
		warnings_p->push_back( std::string("filter program does not exist: ").append(result.second) ) ;
	}
	else if( warnings_p && G::File::isDirectory(result.second,std::nothrow) )
	{
		warnings_p->push_back( std::string("invalid program: ").append(result.second) ) ;
	}
}

