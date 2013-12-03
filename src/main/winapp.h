//
// Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "configuration.h"
#include "output.h"
#include <memory>

/// \namespace Main
namespace Main
{
	class WinApp ;
}

/// \class Main::WinApp
/// An application class instantiated in WinMain() and containing a 
/// Main::WinForm object. 
///
/// WinMain() sets up slot/signal links from Main::Run to Main::WinApp so that 
/// the Main::Run class can emit() progress events like "connecting to ..." and
/// the Main::WinApp class will display them (in the title bar, for instance).
///
/// The class derives from Main::Output so that Main::CommandLine can call
/// output() to throw up message boxes.
///
class Main::WinApp : public GGui::ApplicationBase , public Main::Output 
{
public:
	G_EXCEPTION( Error , "application error" ) ;

	WinApp( HINSTANCE h , HINSTANCE p , const char * name ) ;
		///< Constructor. Initialise with init().

	virtual ~WinApp() ;
		///< Destructor.

	void init( const Main::Configuration & cfg ) ;
		///< Initialises the object after construction.

	int exitCode() const ;
		///< Returns an exit code.

	void disableOutput() ;
		///< Disables subsequent calls to output().

	virtual void output( const std::string & message , bool error ) ;
		///< Puts up a message box. Override from Main::Output.

	void onError( const std::string & message ) ;
		///< To be called when WinMain() catches an exception.

	unsigned int columns() ;
		///< See Main::Output.

	bool confirm() ;
		///< Puts up a confirmation message box.

	void formOk() ;
		///< Called from the form's ok button handler.

	void formDone() ;
		///< Called from the form's nc-destroy message handler.

	void onRunEvent( std::string , std::string , std::string ) ;
		///< Slot for Main::Run::signal().

private:
	void doOpen() ;
	void doClose() ;
	void doQuit() ;
	void hide() ;
	virtual UINT resource() const ;
	virtual DWORD windowStyle() const ;
	virtual bool onCreate() ;
	virtual bool onClose() ;
	virtual void onTrayDoubleClick() ;
	virtual void onTrayRightMouseButtonDown() ;
	virtual void onDimension( int & , int & ) ;
	virtual bool onSysCommand( SysCommand ) ;
	virtual LRESULT onUser( WPARAM , LPARAM ) ;
	void setStatus( const std::string & , const std::string & ) ;

private:
	std::auto_ptr<GGui::Tray> m_tray ;
	std::auto_ptr<Main::WinForm> m_form ;
	std::auto_ptr<Main::Configuration> m_cfg ;
	bool m_quit ;
	bool m_use_tray ;
	bool m_hidden ;
	int m_exit_code ;
} ;

#endif

