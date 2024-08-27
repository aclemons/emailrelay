//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file winform.cpp
///

#include "gdef.h"
#include "gnowide.h"
#include "gssl.h"
#include "gstr.h"
#include "gstringwrap.h"
#include "gmonitor.h"
#include "gtime.h"
#include "ggettext.h"
#include "gconvert.h"
#include "gassert.h"
#include "run.h"
#include "winform.h"
#include "news.h"
#include "licence.h"
#include "legal.h"
#include "resource.h"
#include "glog.h"
#include <cstring>

Main::WinForm::WinForm( HINSTANCE hinstance , const G::StringArray & cfg_data ,
	HWND parent , HWND hnotify , std::pair<DWORD,DWORD> style ,
	bool allow_apply , bool with_icon , bool with_system_menu_quit ) :
		GGui::Stack(*this,hinstance,style) ,
		m_hnotify(hnotify) ,
		m_allow_apply(allow_apply) ,
		m_closed(false) ,
		m_cfg_data(cfg_data)
{
	using G::txt ;

	m_cfg_data.insert( m_cfg_data.begin() , "Value" ) ;
	m_cfg_data.insert( m_cfg_data.begin() , "Key" ) ;
	m_cfg_data.push_back( "tls library" ) ;
	m_cfg_data.push_back( GSsl::Library::ids() ) ;

  	addPage( txt("Configuration") , IDD_PROPPAGE_1 ) ;
  	addPage( txt("Licence") , IDD_PROPPAGE_1 ) ;
  	addPage( txt("Version") , IDD_PROPPAGE_1 ) ;
  	addPage( txt("Status") , IDD_PROPPAGE_1 ) ;

	// create the stack
  	create( parent , "E-MailRelay" , with_icon?IDI_ICON1:0 , hnotify , GGui::Cracker::wm_user_other() ) ;

	if( with_system_menu_quit )
		addSystemMenuItem( txt("Quit") , quitId() ) ;
}

void Main::WinForm::addSystemMenuItem( std::string_view name , unsigned int id )
{
	HMENU hmenu = GetSystemMenu( handle() , FALSE ) ;
	if( hmenu )
		G::nowide::insertMenuItem( hmenu , id , G::sv_to_string(name) ) ;
}

void Main::WinForm::minimise()
{
	ShowWindow( handle() , SW_MINIMIZE ) ;
}

void Main::WinForm::restore()
{
	ShowWindow( handle() , SW_RESTORE ) ;
	BringWindowToTop( handle() ) ;
}

bool Main::WinForm::visible() const
{
	bool minimised = IsIconic( handle() ) != 0 ;
	return !m_closed && !minimised ;
}

void Main::WinForm::close()
{
	G_DEBUG( "Main::WinForm::close: closed=" << (m_closed?1:0) ) ;
	if( !m_closed )
	{
		m_closed = true ;
		DestroyWindow( handle() ) ;
	}
}

bool Main::WinForm::closed() const
{
	return m_closed ;
}

void Main::WinForm::onInit( HWND hdialog , int index )
{
	G_DEBUG( "Main::WinForm::onInit: h=" << hdialog << " index=" << index ) ;
	if( index == 0 ) // "Configuration"
	{
		m_cfg_view = std::make_unique<GGui::ListView>( hdialog , IDC_LIST1 ) ;
  		m_cfg_view->set( m_cfg_data , /*ncolumns=*/2U , /*widthpx=*/150U ) ;
	}
	else if( index == 1 ) // "Licence"
	{
		m_licence_view = std::make_unique<GGui::ListView>( hdialog , IDC_LIST1 ) ;
  		m_licence_view->set( licenceData() , 1U , 330U ) ;
	}
	else if( index == 2 ) // "Version"
	{
		m_version_view = std::make_unique<GGui::ListView>( hdialog , IDC_LIST1 ) ;
  		m_version_view->set( versionData() , 1U , 330U ) ;
	}
	else if( index == 3 ) // "Status"
	{
		m_status_view = std::make_unique<GGui::ListView>( hdialog , IDC_LIST1 ) ;
  		m_status_view->set( statusData() , 3U , 100U ) ;
	}
}

bool Main::WinForm::onApply()
{
	// called by the Stack when the property page's main apply button
	// ("Close") is pressed -- if false is returned then the Stack will
	// not complete the dialog but post an apply-denied notification
	// message to the WinApp instead
	return m_allow_apply ;
}

void Main::WinForm::setStatus( const std::string & s0 , const std::string & s1 ,
	const std::string & s2 , const std::string & s3 )
{
	G_ASSERT( s0 == "client" || s0 == "network" ) ;
	G_ASSERT( ( s0 == "client" && ( s1 == "forward" || s1 == "resolving" || s1 == "connecting" || s1 == "connected" || s1 == "sending" || s1 == "sent" ) ) ||
		( s0 == "network" && ( s1 == "in" || s1 == "out" || s1 == "listen" ) ) ) ;
	G_DEBUG( "Main::WinForm::setStatus: [" << s0 << "] [" << s1 << "] [" << s2 << "] [" << s3 << "]" ) ;
	G_DEBUG( "Main::WinForm::setStatus: time=[" << G::Time(G::Time::LocalTime()).hhmmss(":") << "]" ) ;

	// client forward {start|end <error>}
	// client {connecting|resolving|connected|sending|sent} {<address>|<msgid>}
	// network {in|out|listen} {start|stop}
	// store update
	//
	if( s0 == "client" )
	{
		if( s1 == "forward" && s2 == "start" )
		{
			m_status_map["Forwarding"] = std::make_pair( timestamp() , "started" ) ;
		}
		else if( s1 == "forward" && s2 == "end" )
		{
			std::string reason = G::Str::printable( s3 ) ;
			m_status_map["Forwarding"] = std::make_pair( timestamp() ,
				reason.empty() ? std::string("finished") : reason ) ;
		}
		else if( s1 == "sending" )
		{
			const std::string & message_id = s2 ;
			m_status_map["Message"] = std::make_pair( timestamp() ,
				message_id + " (sending)" ) ;
		}
		else if( s1 == "sent" )
		{
			const std::string & message_id = s2 ;
			std::string reason = G::Str::printable( s3 ) ;
			m_status_map["Message"] = std::make_pair( timestamp() ,
				message_id + " (" + (reason.empty()?std::string("sent"):reason) + ")" ) ;
		}
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

std::string Main::WinForm::timestamp()
{
	return G::Time(G::Time::LocalTime()).hhmmss(":") ;
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
	add( s , G::StringWrap::wrap(Main::News::text(""),"","",60U) ) ;
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
	G::Str::splitIntoFields( s , list , '\n' ) ;
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

