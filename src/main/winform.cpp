//
// Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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
//
// winform.cpp
//

#include "gdef.h"
#include "gssl.h"
#include "gstr.h"
#include "gstrings.h"
#include "gmonitor.h"
#include "gpump.h"
#include "gtime.h"
#include "run.h"
#include "winform.h"
#include "news.h"
#include "licence.h"
#include "winapp.h"
#include "legal.h"
#include "resource.h"
#include <sstream>

Main::WinForm::WinForm( WinApp & app , const Main::Configuration & cfg , bool /*confirm*/ ) :
	GGui::Stack(*this,app.hinstance()) ,
	m_app(app) ,
	m_closed(false) ,
	m_cfg(cfg)
{
  	addPage( "Configuration" , IDD_PROPPAGE_1 ) ;
  	addPage( "Licence" , IDD_PROPPAGE_1 ) ;
  	addPage( "Version" , IDD_PROPPAGE_1 ) ;
  	addPage( "Status" , IDD_PROPPAGE_1 ) ;

	// create the stack - completion notification is via App::onUserOther()
  	create( app.handle() , "E-MailRelay" , 0 , GGui::Cracker::wm_user_other() ) ; // GGui::Stack
}

void Main::WinForm::close()
{
	G_DEBUG( "Main::WinForm::close: closed=" << (m_closed?1:0) ) ;
	if( !m_closed )
	{
		m_closed = true ;
		::DestroyWindow( handle() ) ;
	}
}

bool Main::WinForm::closed() const
{
	return m_closed ;
}

void Main::WinForm::onInit( HWND hdialog , const std::string & title )
{
	if( title == "Configuration" )
	{
		m_cfg_view.reset( new GGui::ListView(hdialog,IDC_LIST1) ) ;
  		m_cfg_view->set( cfgData() , 2U , 150U ) ;
	}
	else if( title == "Version" )
	{
		m_version_view.reset( new GGui::ListView(hdialog,IDC_LIST1) ) ;
  		m_version_view->set( versionData() , 1U , 330U ) ;
	}
	else if( title == "Status" )
	{
		m_status_view.reset( new GGui::ListView(hdialog,IDC_LIST1) ) ;
  		m_status_view->set( statusData() , 3U , 100U ) ;
	}
	else if( title == "Licence" )
	{
		m_licence_view.reset( new GGui::ListView(hdialog,IDC_LIST1) ) ;
  		m_licence_view->set( licenceData() , 1U , 330U ) ;
	}
}

void Main::WinForm::onDestroy( HWND )
{
	G_DEBUG( "Main::WinForm::onDestroy: on destroy" ) ;
}

void Main::WinForm::setStatus( const std::string & category , const std::string & s1 , const std::string & s2 )
{
	G_DEBUG( "Main::WinForm::setStatus: [" << category << "] [" << s1 << "] [" << s2 << "]" ) ;
	G_DEBUG( "Main::WinForm::setStatus: time=[" << G::Time(G::Time::LocalTime()).hhmmss(":") << "]" ) ;

	// poll,{start|busy|end,<error>}
	// forward,{start|busy|end,<error>}
	// client,{sending,<message>|connecting,<address>|connected,<address>|done,""|failed,<error>}
	// store,update,[poll]
	// network,{in|out},{start|end}
	//
	if( ( category == "poll" || category == "forward" ) && s1 == "start" )
	{
		m_status_map["Forwarding started"] = std::make_pair(G::Time(G::Time::LocalTime()).hhmmss(":"),std::string()) ;
	}
	else if( ( category == "poll" || category == "forward" ) && s1 == "end" )
	{
		m_status_map["Forwarding finished"] = std::make_pair(G::Time(G::Time::LocalTime()).hhmmss(":"),s2) ;
	}
	else if( category == "client" && s1 == "sending" )
	{
		m_status_map["Forwarding file"] = std::make_pair(G::Time(G::Time::LocalTime()).hhmmss(":"),s2) ;
	}
	else if( category == "client" && s1 == "failed" )
	{
		m_status_map["Connection failure"] = std::make_pair(G::Time(G::Time::LocalTime()).hhmmss(":"),s2) ;
	}

	// update the gui
	if( !m_closed && m_status_view.get() )
	{
		G::StringArray status_data ;
		status_data.reserve( 20U ) ;
		getStatusData( status_data ) ;
		m_status_view->update( status_data , 3U ) ;
	}
}

G::StringArray Main::WinForm::versionData() const
{
	G::StringArray s ;
	s.reserve( 50U ) ;
	add( s , Main::Run::versionNumber() ) ;
	add( s , "E-MailRelay V" + Main::Run::versionNumber() ) ;
	add( s , "" ) ;
	add( s , Main::Legal::copyright() ) ;
	add( s , "" ) ;
	if( !GSsl::Library::credit("","\n","").empty() )
	{
		add( s , GSsl::Library::credit("","\n","") ) ;
		add( s , "" ) ;
	}
	add( s , Main::Legal::warranty("","\n") ) ;
	add( s , "" ) ;
	add( s , G::Str::wrap(Main::News::text(""),"","",60U) ) ;
	return s ;
}

G::StringArray Main::WinForm::licenceData() const
{
	G::StringArray s ;
	s.reserve( 680U ) ;
	add( s , "GPLv3" ) ;
	for( const char **p = licence ; *p ; p++ )
		add( s , *p ) ;
	return s ;
}

G::StringArray Main::WinForm::cfgData() const
{
	const Configuration & c = m_cfg ;
	G::StringArray s = m_cfg.display() ;
	s.insert( s.begin() , "Value" ) ;
	s.insert( s.begin() , "Key" ) ;
	s.push_back( "tls library" ) ;
	s.push_back( GSsl::Library::ids() ) ;
	return s ;
}

void Main::WinForm::getStatusData( G::StringArray & out ) const
{
	// headings
	out.push_back( "Status" ) ;
	out.push_back( "" ) ;
	out.push_back( "" ) ;

	// m_status_map
	for( StatusMap::const_iterator p = m_status_map.begin() ; p != m_status_map.end() ; ++p )
	{
		out.push_back( (*p).first ) ;
		out.push_back( (*p).second.first ) ;
		out.push_back( (*p).second.second ) ;
	}

	// gnet monitor report
	if( GNet::Monitor::instance() )
		GNet::Monitor::instance()->report( out ) ;
}

G::StringArray Main::WinForm::statusData() const
{
	G::StringArray s ;
	getStatusData( s ) ;
	return s ;
}

G::StringArray Main::WinForm::split( const std::string & s )
{
	G::StringArray list ;
	G::Str::splitIntoFields( s , list , "\n" ) ;
	return list ;
}

void Main::WinForm::add( G::StringArray & list , const std::string & key , const std::string & value )
{
	list.push_back( key ) ;
	list.push_back( value ) ;
}

void Main::WinForm::add( G::StringArray & list , std::string s )
{
	G::Str::trim( s , "\r\n" ) ;
	if( s.empty() )
		list.push_back( std::string() ) ;
	G::StringArray lines = split( s ) ;
	for( G::StringArray::iterator p = lines.begin() ; p != lines.end() ; ++p )
		list.push_back( *p ) ;
}

/// \file winform.cpp
