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
/// \file filterfactory.cpp
///

#include "gdef.h"
#include "filterfactory.h"
#include "run.h"
#include "unit.h"
#include "gstr.h"

#if !defined(FILTER_DEMO) && defined(GCONFIG_FILTER_MASK) && (GCONFIG_FILTER_MASK & 0x08)
#define FILTER_DEMO 1
#endif

#ifdef FILTER_DEMO
#include "demofilter.h"
#endif

Main::FilterFactory::FilterFactory( Run & run , Unit & unit , GStore::FileStore & store ) :
	GFilters::FilterFactory(store) ,
	m_run(run) ,
	m_unit(unit) ,
	m_store(store)
{
	GDEF_IGNORE_VARIABLE( m_run ) ;
	GDEF_IGNORE_VARIABLE( m_unit ) ;
	GDEF_IGNORE_VARIABLE( m_store ) ;
}

GFilters::FilterFactory::Spec Main::FilterFactory::parse( const std::string & spec ,
	const G::Path & base_dir , const G::Path & app_dir ,
	G::StringArray * warnings_p )
{
	#ifdef FILTER_DEMO
		if( !spec.empty() && spec.find(',') == std::string::npos && spec.find("demo:") == 0U )
			return Spec( "demo" , G::Str::tail(spec,":") ) ;
	#endif
	return GFilters::FilterFactory::parse( spec , base_dir , app_dir , warnings_p ) ;
}

std::unique_ptr<GSmtp::Filter> Main::FilterFactory::newFilter( GNet::ExceptionSink es ,
	bool server_side , const Spec & spec , unsigned int timeout ,
	const std::string & log_prefix )
{
	#ifdef FILTER_DEMO
		if( spec.first == "demo" )
			return std::make_unique<DemoFilter>( es , m_run , m_unit.id() , m_store , spec.second ) ;
	#endif
	return GFilters::FilterFactory::newFilter( es , server_side , spec , timeout , log_prefix ) ;
}

