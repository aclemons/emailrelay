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
/// \file winapp.cpp
///

#include "gdef.h"
#include "winapp.h"
#include "winmenu.h"
#include "gwindow.h"
#include "glog.h"
#include "gstr.h"
#include "gscope.h"
#include "gtest.h"
#include "gcontrol.h"
#include "gdialog.h"
#include "ggettext.h"
#include "resource.h"

namespace Main
{
	class Box ;
	class Config ;
	struct PixelLayout
	{
		explicit PixelLayout( bool verbose ) ;
		int tabstop () const { return m_tabstop ; }
		unsigned int width() const { return m_width ; }
		unsigned int width2() const { return m_width2 ; }
		private:
		static bool isWine() ;
		int m_tabstop ;
		unsigned int m_width ;
		unsigned int m_width2 ;
	} ;
}

Main::PixelLayout::PixelLayout( bool verbose )
{
	m_tabstop = verbose ? 122 : 90 ;
	m_width = verbose ? 60U : 80U ;
	m_width2 = verbose ? 48U : 80U ;
}

bool Main::PixelLayout::isWine()
{
	HMODULE h = GetModuleHandle( "ntdll.dll" ) ;
	return h && !!GetProcAddress( h , "wine_get_version" ) ;
}

class Main::Box : public GGui::Dialog
{
public:
	Box( GGui::ApplicationBase & app , const G::StringArray & text , int tabstop ) ;
	bool run() ;

private: // overrides
	bool onInit() override ; // Override from GGui::Dialog.

private:
	GGui::EditBox m_edit ;
	G::StringArray m_text ;
	int m_tabstop ;
} ;

// ==

Main::WinApp::WinApp( HINSTANCE h , HINSTANCE p , const std::string & name ) :
	GGui::ApplicationBase(h,p,name) ,
	m_disable_output(false) ,
	m_quitting(false) ,
	m_exit_code(0) ,
	m_in_do_open(false) ,
	m_in_do_close(false)
{
}

Main::WinApp::~WinApp()
{
}

void Main::WinApp::disableOutput()
{
	m_disable_output = true ;
}

void Main::WinApp::init( const Configuration & configuration , const G::Options & options_spec )
{
	m_configuration_data = configuration.display( options_spec ) ;
	m_cfg = Config::create( configuration ) ;
}

int Main::WinApp::exitCode() const
{
	// see test/Server.pm hasDebug()
	if( G::Test::enabled("special-exit-code") )
		return (G::threading::works()?23:25) ;

	return m_exit_code ;
}

std::pair<DWORD,DWORD> Main::WinApp::windowStyle() const
{
	return GGui::Window::windowStyleMain() ;
}

DWORD Main::WinApp::classStyle() const
{
	return 0 ;
}

UINT Main::WinApp::resource() const
{
	// (resource() provides the combined menu and icon id, but we have no menus)
	return IDI_ICON1 ;
}

bool Main::WinApp::onCreate()
{
	if( m_cfg.with_tray )
	{
		try
		{
			m_tray = std::make_unique<GGui::Tray>( resource() , *this , "E-MailRelay" ) ;
		}
		catch( std::exception & e )
		{
			using G::txt ;
			throw G::Exception( e.what() , txt("try using the --hidden option") ) ;
		}
	}
	if( m_cfg.open_on_create )
	{
		doOpen() ;
	}
	return true ;
}

namespace Main
{
	struct ScopeExitReset
	{
		ScopeExitReset( std::unique_ptr<Main::WinMenu> & ptr ) : m_ptr(ptr) {}
		~ScopeExitReset() { m_ptr.reset() ; }
		std::unique_ptr<Main::WinMenu> & m_ptr ;
	} ;
}

void Main::WinApp::onTrayRightMouseButtonDown()
{
	G_DEBUG( "Main::WinApp::onTrayRightMouseButtonDown: tray right-click" ) ;

	// (popup() returns when the mouse released, but we might get
	// other event notifications before then, so make the menu
	// accessible to them via a data member)
	ScopeExitReset _( m_menu ) ;
	m_menu = std::make_unique<WinMenu>( IDR_MENU1 ) ;

	bool form_is_visible = m_form.get() != nullptr && m_form.get()->visible() ;
	bool with_open = !form_is_visible ;
	bool with_close = form_is_visible ;
	int id = m_menu->popup( *this , false , with_open , with_close ) ;

	PostMessage( handle() , wm_user() , 1 , static_cast<LPARAM>(id) ) ;
}

void Main::WinApp::onTrayDoubleClick()
{
	G_DEBUG( "Main::WinApp::onTrayDoubleClick: tray double-click" ) ;
	PostMessage( handle() , wm_user() , 2 , static_cast<LPARAM>(IDM_OPEN) ) ;
}

LRESULT Main::WinApp::onUser( WPARAM /*wparam*/ , LPARAM lparam )
{
	G_DEBUG( "Main::WinApp::onUser: lparam=" << lparam ) ;
	int id = static_cast<int>(lparam) ;
	if( id == IDM_OPEN ) doOpen() ;
	if( id == IDM_CLOSE ) doClose() ;
	if( id == IDM_QUIT ) doQuit() ;
	return 0L ;
}

LRESULT Main::WinApp::onUserOther( WPARAM wparam , LPARAM )
{
	// this is asynchronous notification from GGui::Stack that
	// the dialog has completed (wparam=0/1) or the apply button
	// has been denied (wparam=2) or WM_SYSCOMMAND has been
	// received (wparam=3)

	G_DEBUG( "Main::WinApp::onUserOther: wparam=" << wparam ) ;

	if( wparam == 3 ) // iff cfg.with_sysmenu_quit
		doQuit() ;

	else if( m_cfg.quit_on_form_ok )
		doQuit() ;

	else if( m_cfg.close_on_form_ok )
		doClose() ;

	return 0L ;
}

void Main::WinApp::doOpen()
{
	G_DEBUG( "Main::WinApp::doOpen: do-open" ) ;
	if( m_in_do_open || m_in_do_close ) return ;
	G::ScopeExitSetFalse _( m_in_do_open ) ;

	if( m_cfg.never_open )
		return ;

	if( m_form.get() == nullptr || m_form.get()->closed() )
	{
		G_DEBUG( "Main::WinApp::doOpen: do-open: form reset" ) ;

		std::pair<DWORD,DWORD> form_style( WS_OVERLAPPEDWINDOW , 0 ) ;
		if( m_cfg.form_minimisable )
		{
			form_style.first &= ~WS_MAXIMIZEBOX ;
			form_style.second = WS_EX_APPWINDOW ;
		}
		else
		{
			form_style.first &= ~WS_MAXIMIZEBOX ;
			form_style.first &= ~WS_MINIMIZEBOX ;
			form_style.first &= ~WS_SYSMENU ;
		}

		HWND form_hparent = handle() ;
		if( m_cfg.form_parentless )
			form_hparent = 0 ;

		bool form_allow_apply = m_cfg.allow_apply ;
		bool form_with_icon = true ;
		bool form_with_system_menu_quit = m_cfg.with_sysmenu_quit ;

		m_form = std::make_unique<WinForm>( hinstance() , m_configuration_data ,
			form_hparent , handle() , form_style , form_allow_apply ,
			form_with_icon , form_with_system_menu_quit ) ;
	}

	if( m_cfg.restore_on_open )
	{
		if( m_form.get() != nullptr )
			m_form->restore() ;
	}
}

void Main::WinApp::doClose()
{
	G_DEBUG( "Main::WinApp::doClose: do-close" ) ;
	if( m_in_do_open || m_in_do_close ) return ;
	G::ScopeExitSetFalse _( m_in_do_close ) ;

	if( m_form.get() != nullptr )
	{
		if( m_cfg.minimise_on_close )
			m_form->minimise() ;

		if( m_cfg.close_on_close )
			m_form->close() ;
	}
}

void Main::WinApp::doQuit()
{
	G_DEBUG( "Main::WinApp::doQuit: do-quit" ) ;
	m_quitting = true ;
	close() ; // AppBase::close() so WM_CLOSE and virtual onClose()
}

bool Main::WinApp::onClose()
{
	G_DEBUG( "Main::WinApp::onClose: on-close" ) ;
	if( m_quitting )
	{
		return true ; // continue to WM_DESTROY etc
	}
	else if( m_tray.get() != nullptr )
	{
		doClose() ;
		return false ; // dont continue with the WM_CLOSE
	}
	else
	{
		return true ;
	}
}

void Main::WinApp::onRunEvent( std::string s0 , std::string s1 , std::string s2 , std::string s3 )
{
	if( m_form.get() )
		m_form->setStatus( s0 , s1 , s2 , s3 ) ;
}

void Main::WinApp::onWindowException( std::exception & e )
{
	GGui::Window::onWindowException( e ) ;
}

G::OptionsUsage::Config Main::WinApp::outputLayout( bool verbose ) const
{
	G::OptionsUsage::Config layout ;
	layout.separator = "\t" ;
	layout.width = PixelLayout(verbose).width() ;
	layout.width2 = PixelLayout(verbose).width2() ;
	layout.margin = 0U ;
	return layout ;
}

bool Main::WinApp::outputSimple() const
{
	return false ;
}

void Main::WinApp::output( const std::string & text , bool , bool verbose )
{
	if( !m_disable_output )
	{
		G::StringArray text_lines ;
		G::Str::splitIntoFields( G::Str::removedAll(text,'\r') , text_lines , '\n' ) ;
		if( text_lines.size() > 10U ) // eg. "--help"
		{
			Box box( *this , text_lines , PixelLayout(verbose).tabstop() ) ;
			if( ! box.run() )
				messageBox( text ) ;
		}
		else
		{
			messageBox( text ) ;
		}
	}
}

void Main::WinApp::onError( const std::string & text , int exit_code )
{
	// called from WinMain(), possibly before init()
	output( text , true , false ) ; // override implemented above
	m_exit_code = exit_code ;
}

// ==

Main::Box::Box( GGui::ApplicationBase & app , const G::StringArray & text , int tabstop ) :
	GGui::Dialog(app,false) ,
	m_edit(*this,IDC_EDIT1) ,
	m_text(text) ,
	m_tabstop(tabstop)
{
}

bool Main::Box::onInit()
{
	G_DEBUG( "Main::Box::onInit" ) ;

	std::vector<int> tabs ;
	tabs.push_back( m_tabstop ) ;
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

// ==

Main::WinApp::Config Main::WinApp::Config::hidden()
{
	Config cfg ;
	cfg.never_open = true ;
	return cfg ;
}

Main::WinApp::Config Main::WinApp::Config::tray()
{
	Config cfg ;
	cfg.open_on_create = false ;
	cfg.with_tray = true ;
	cfg.close_on_form_ok = true ;
	cfg.close_on_close = true ;
	return cfg ;
}

Main::WinApp::Config Main::WinApp::Config::nodaemon()
{
	Config cfg ;
	cfg.quit_on_form_ok = true ;
	cfg.close_on_close = true ;
	return cfg ;
}

Main::WinApp::Config Main::WinApp::Config::window( bool with_tray )
{
	Config cfg ;
	cfg.with_tray = with_tray ;
	cfg.with_sysmenu_quit = true ;
	cfg.close_on_form_ok = true ;
	cfg.form_minimisable = true ;
	cfg.form_parentless = true ;
	cfg.minimise_on_close = true ;
	cfg.restore_on_open = true ;
	return cfg ;
}

Main::WinApp::Config Main::WinApp::Config::create( const Main::Configuration & configuration )
{
	if( configuration.hidden() )
	{
		// "--hidden" for no window, no tray icon and no message boxes
		return hidden() ;
	}
	else if( !configuration.daemon() )
	{
		// "--no-daemon" for a foreground window with no taskbar button and no tray icon; close terminates
		return nodaemon() ;
	}
	else if( configuration.show("window") )
	{
		// "--show=window" or "--show=window,tray" for a minimisable window and with a sysmenu quit item; close minimises
		return window( configuration.show("tray") ) ;
	}
	else if( configuration.show("nodaemon") || configuration.show("popup") )
	{
		// "--show=popup" or "--show=nodaemon" are like "--no-daemon", ie. a foreground window with no taskbar button and no tray icon; close terminates
		return nodaemon() ;
	}
	else if( configuration.show("hidden") )
	{
		// "--show=hidden" is like "--hidden"
		return hidden() ;
	}
	else if( configuration.show("tray") )
	{
		// "--show=tray" for a foreground window hidden/shown by a tray icon; close hides
		return tray() ;
	}
	else
	{
		return tray() ;
	}
}

