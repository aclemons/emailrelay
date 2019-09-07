//
// Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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

#ifndef WIN_APP_H
#define WIN_APP_H

#include "gdef.h"
#include "gappbase.h"
#include "gexception.h"
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

/// \class Main::WinApp
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
	G_EXCEPTION( Error , "application error" ) ;

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
	virtual void output( const std::string & message , bool error ) override ; // Override from Main::Output.
	virtual G::Options::Layout layout() const override ; // Override from Main::Output.
	virtual bool simpleOutput() const override ; // Override from Main::Output.
	virtual void onWindowException( std::exception & e ) override ; // Override from GGui::Window.
	virtual DWORD classStyle() const override ; // Override from GGui::ApplicationBase.
	virtual std::pair<DWORD,DWORD> windowStyle() const override ; // Override from GGui::ApplicationBase.
	virtual UINT resource() const override ; // Override from GGui::ApplicationBase.
	virtual bool onCreate() override ;
	virtual bool onClose() override ;
	virtual void onTrayDoubleClick() override ;
	virtual void onTrayRightMouseButtonDown() override ;
	virtual LRESULT onUser( WPARAM , LPARAM ) override ;
	virtual LRESULT onUserOther( WPARAM , LPARAM ) override ;

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

private:
	WinApp( const WinApp & ) g__eq_delete ;
	void operator=( const WinApp & ) g__eq_delete ;
	void doOpen() ;
	void doClose() ;
	void doQuit() ;

private:
	unique_ptr<GGui::Tray> m_tray ;
	unique_ptr<Main::WinForm> m_form ;
	unique_ptr<Main::Configuration> m_form_cfg ;
	unique_ptr<Main::WinMenu> m_menu ;
	bool m_disable_output ;
	Config m_cfg ;
	bool m_quitting ;
	int m_exit_code ;
	bool m_in_do_open ;
	bool m_in_do_close ;
} ;

#endif
