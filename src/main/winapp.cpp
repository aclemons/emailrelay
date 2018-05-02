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
// winapp.cpp
//

#include "gdef.h"
#include "winapp.h"
#include "winmenu.h"
#include "gwindow.h"
#include "glog.h"
#include "gstr.h"
#include "gtest.h"
#include "gcontrol.h"
#include "gdialog.h"
#include "gassert.h"
#include "resource.h"
#if G_MINGW
static int cfg_tab_stop = 200 ; // edit control tab stops are flakey in mingw
#else
static int cfg_tab_stop = 120 ;
#endif

namespace Main
{
	class Box ;
} ;

/// \class Main::Box
/// A message-box class for pop-up messages on windows.
///
class Main::Box : public GGui::Dialog
{
public:
	Box( GGui::ApplicationBase & app , const G::StringArray & text ) ;
	bool run() ;

private:
	bool onInit() ;

private:
	GGui::EditBox m_edit ;
	G::StringArray m_text ;
} ;

Main::Box::Box( GGui::ApplicationBase & app , const G::StringArray & text ) :
	GGui::Dialog( app , false ) ,
	m_edit( *this , IDC_EDIT1 ) ,
	m_text( text )
{
}

bool Main::Box::onInit()
{
	G_DEBUG( "Main::Box::onInit" ) ;

	std::vector<int> tabs ;
	tabs.push_back( 10 ) ;
	tabs.push_back( cfg_tab_stop ) ;
	m_edit.setTabStops( tabs ) ;

	m_edit.set( m_text ) ;
	return true ;
}

bool Main::Box::run()
{
	GGui::Dialog & base = *this ;
	bool rc = base.run( IDD_DIALOG2 ) ;
	G_DEBUG( "Main::Box::run: " << rc ) ;
	return rc ;
}

// ===

Main::WinApp::WinApp( HINSTANCE h , HINSTANCE p , const char * name ) :
	GGui::ApplicationBase( h , p , name ) ,
	m_quit(false) ,
	m_use_tray(false) ,
	m_hidden(false) ,
	m_exit_code(0)
{
}

Main::WinApp::~WinApp()
{
}

void Main::WinApp::disableOutput()
{
	m_hidden = true ;
}

void Main::WinApp::init( const Configuration & cfg )
{
	m_use_tray = cfg.daemon() ;
	m_cfg.reset( new Configuration(cfg) ) ;
	m_hidden = m_hidden || m_cfg->hidden() ;
}

int Main::WinApp::exitCode() const
{
	return G::Test::enabled("special-exit-code") ? (G::threading::works()?23:25) : m_exit_code ;
}

void Main::WinApp::hide()
{
	show( SW_HIDE ) ;
}

DWORD Main::WinApp::windowStyle() const
{
	return 0 ; // not visible
}

UINT Main::WinApp::resource() const
{
	// (resource() provides the combined menu and icon id, but we have no menus)
	return IDI_ICON1 ;
}

bool Main::WinApp::onCreate()
{
	if( m_use_tray )
		m_tray.reset( new GGui::Tray(resource(),*this,"E-MailRelay") ) ;
	else
		doOpen() ;
	return true ;
}

void Main::WinApp::onTrayRightMouseButtonDown()
{
	WinMenu menu( IDR_MENU1 ) ;
	bool form_is_open = m_form.get() != nullptr && !m_form.get()->closed() ;
	bool with_open = !form_is_open ;
	bool with_close = form_is_open ;
	int id = menu.popup( *this , with_open , with_close ) ;

	// make it asychronous to prevent "RPC_E_CANTCALLOUT_ININPUTSYNCCALL" --
	// see App::onUser()
	::PostMessage( handle() , wm_user() , 0 , static_cast<LPARAM>(id) ) ;
}

void Main::WinApp::onTrayDoubleClick()
{
	// make it asychronous to prevent "RPC_E_CANTCALLOUT_ININPUTSYNCCALL" --
	// see App::onUser()
	::PostMessage( handle() , wm_user() , 0 , static_cast<LPARAM>(IDM_OPEN) ) ;
}

LRESULT Main::WinApp::onUserOther( WPARAM , LPARAM lparam )
{
	G_DEBUG( "Main::WinApp::onUserOther: hwnd=" << reinterpret_cast<HWND>(lparam) << " form=" << (m_form.get()?m_form->handle():HWND(0)) ) ;
	HWND hwnd = reinterpret_cast<HWND>( lparam ) ;
	if( m_form.get() != nullptr && hwnd == m_form->handle() ) // just in case delayed in the queue
		m_form->close() ;
	return 0L ;
}

LRESULT Main::WinApp::onUser( WPARAM , LPARAM lparam )
{
	int id = static_cast<int>(lparam) ;
	if( id == IDM_OPEN ) doOpen() ;
	if( id == IDM_CLOSE ) doClose() ;
	if( id == IDM_QUIT ) doQuit() ;
	return 0L ;
}

void Main::WinApp::doOpen()
{
	if( m_form.get() == nullptr || m_form.get()->closed() )
	{
		m_form.reset( new WinForm( *this , *m_cfg.get() , /*confirm=*/!m_use_tray ) ) ;
	}

	show() ;
}

void Main::WinApp::doQuit()
{
	m_quit = true ;
	close() ; // AppBase::close() -- triggers onClose(), but without doClose()
}

bool Main::WinApp::onClose()
{
	// (this is triggered by AppBase::close() or using the system close menu item)

	if( m_quit || m_hidden )
	{
		return true ;
	}
	else if( m_use_tray )
	{
		doClose() ;
		return false ; // dont continue with the WM_CLOSE, just hide
	}
	else
	{
		return confirm() ;
	}
}

bool Main::WinApp::confirm()
{
	// (also called from winform)
	return true ; // was messageBoxQuery("really?")
}

void Main::WinApp::doClose()
{
	hide() ;

	// (close the form so that it gets recreated each time with current data)
	if( m_form.get() != nullptr )
		m_form->close() ;
}

void Main::WinApp::formOk()
{
	// (this is triggered by clicking the OK button)
	m_use_tray ? doClose() : doQuit() ;
}

bool Main::WinApp::onSysCommand( SysCommand sc )
{
	// true <= processed as no-op => dont change size
	return sc == scMaximise || sc == scSize ;
}

void Main::WinApp::onRunEvent( std::string category , std::string s1 , std::string s2 )
{
	if( m_form.get() )
		m_form->setStatus( category , s1 , s2 ) ;
}

void Main::WinApp::onWindowException( std::exception & e )
{
	GGui::Window::onWindowException( e ) ;
}

G::Options::Layout Main::WinApp::layout() const
{
	G::Options::Layout layout( 0U ) ;
	layout.separator = "\t" ;
	layout.indent = "\t\t" ;
	layout.width = 100U ;
	return layout ;
}

bool Main::WinApp::simpleOutput() const
{
	return false ;
}

void Main::WinApp::output( const std::string & text , bool )
{
	if( !m_hidden )
	{
		G::StringArray text_lines ;
		G::Str::splitIntoFields( text , text_lines , "\r\n" ) ;
		if( text_lines.size() > 25U ) // eg. "--help"
		{
			Box box( *this , text_lines ) ;
			if( ! box.run() )
				messageBox( text ) ;
		}
		else
		{
			messageBox( text ) ;
		}
	}
}

void Main::WinApp::onError( const std::string & text )
{
	// called from WinMain(), possibly before init()
	output( text , true ) ;
	m_exit_code = 1 ;
}

