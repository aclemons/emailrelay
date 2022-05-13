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
/// \file winapp.h
///

#ifndef G_MAIN_WIN_APP_H
#define G_MAIN_WIN_APP_H

#include "gdef.h"
#include "gappbase.h"
#include "gexception.h"
#include "goptionsoutput.h"
#include "gtray.h"
#include "winform.h"
#include "winmenu.h"
#include "configuration.h"
#include "output.h"
#include <memory>
#include <utility>

namespace Main
{
	class WinApp ;
}

//| \class Main::WinApp
/// A main-window class for an invisible window that manages the
/// Main::WinForm user interface, the system tray icon, and message
/// boxes.
///
/// The class derives from Main::Output so that Main::CommandLine can call
/// output() to throw up message boxes.
///
/// The onRunEvent() method is provided as a sink for Main::Run::signal().
///
/// \code
/// WinMain( hinstance , hprevious , show )
/// {
///   WinApp app( hinstance , hprevious , "Test" ) ;
///   if( cfg.hidden() ) app.disableOutput() ;
///   app.init( cfg ) ;
///   app.createWindow( show ) ; // GGui::ApplicationBase
///   EventLoop::run() ; // hooks into GGui::Pump
/// }
/// \endcode
///
class Main::WinApp : public GGui::ApplicationBase , public Main::Output
{
public:
	G_EXCEPTION( Error , tx("application error") ) ;

	WinApp( HINSTANCE h , HINSTANCE p , const std::string & name ) ;
		///< Constructor. Initialise with init().

	virtual ~WinApp() ;
		///< Destructor.

	void init( const Main::Configuration & cfg ) ;
		///< Initialises the object after construction.

	int exitCode() const ;
		///< Returns an exit code.

	void disableOutput() ;
		///< Disables subsequent calls to output().

	void onError( const std::string & message ) ;
		///< To be called when WinMain() catches an exception.

	unsigned int columns() ;
		///< See Main::Output.

	void onRunEvent( std::string , std::string , std::string , std::string ) ;
		///< The Main::Run::signal() signal can be connected to
		///< this method so that its interesting event information
		///< can be displayed.

private: // overrides
	void output( const std::string & message , bool error , bool ) override ; // Override from Main::Output.
	G::OptionsOutput::Layout outputLayout( bool verbose ) const override ; // Override from Main::Output.
	bool outputSimple() const override ; // Override from Main::Output.
	void onWindowException( std::exception & e ) override ; // Override from GGui::Window.
	DWORD classStyle() const override ; // Override from GGui::ApplicationBase.
	std::pair<DWORD,DWORD> windowStyle() const override ; // Override from GGui::ApplicationBase.
	UINT resource() const override ; // Override from GGui::ApplicationBase.
	bool onCreate() override ;
	bool onClose() override ;
	void onTrayDoubleClick() override ;
	void onTrayRightMouseButtonDown() override ;
	LRESULT onUser( WPARAM , LPARAM ) override ;
	LRESULT onUserOther( WPARAM , LPARAM ) override ;

private:
	struct Config
	{
		bool with_tray ;
		bool with_sysmenu_quit ;
		bool never_open ;
		bool open_on_create ;
		bool allow_apply ;
		bool quit_on_form_ok ;
		bool close_on_form_ok ;
		bool close_on_close ;
		bool form_minimisable ;
		bool form_parentless ;
		bool minimise_on_close ;
		bool restore_on_open ;
		Config() ;
		static Config create( const Main::Configuration & ) ;
		static Config hidden() ;
		static Config nodaemon() ;
		static Config window( bool with_tray ) ;
		static Config tray() ;
	} ;

public:
	WinApp( const WinApp & ) = delete ;
	WinApp( WinApp && ) = delete ;
	void operator=( const WinApp & ) = delete ;
	void operator=( WinApp && ) = delete ;

private:
	void doOpen() ;
	void doClose() ;
	void doQuit() ;

private:
	std::unique_ptr<GGui::Tray> m_tray ;
	std::unique_ptr<Main::WinForm> m_form ;
	std::unique_ptr<Main::Configuration> m_form_cfg ;
	std::unique_ptr<Main::WinMenu> m_menu ;
	bool m_disable_output ;
	Config m_cfg ;
	bool m_quitting ;
	int m_exit_code ;
	bool m_in_do_open ;
	bool m_in_do_close ;
} ;

#endif
