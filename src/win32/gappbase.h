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
/// \file gappbase.h
///

#ifndef G_APPBASE_H
#define G_APPBASE_H

#include "gdef.h"
#include "gwindow.h"
#include "gappinst.h"
#include "gexception.h"

/// \namespace GGui
namespace GGui
{
	class ApplicationBase ;
}

/// \class GGui::ApplicationBase
///
/// A simple windows application class which
/// can be used for very-simple applications on its own,
/// or as a base class for the more fully-functional
/// GGui::Application class. It has no dependence on other
/// low-level classes such as G::Path, G::Arg, and (depending 
/// on the build) GGui::Dialog (see "gpump_nodialog.cpp").
///
/// The application object creates and manages the application's 
/// main window and runs the ::GetMessage() event loop.
///
/// GGui::ApplicationBase derives from GGui::Window allowing the
/// user to override the default message handing for the main
/// application window.
///
/// \see Application, ApplicationInstance, Pump
///
class GGui::ApplicationBase : public GGui::ApplicationInstance , public GGui::Window 
{
public:
	G_EXCEPTION( RegisterError, "cannot register application's window class" ) ;
	G_EXCEPTION( CreateError , "cannot create application window" ) ;

	ApplicationBase( HINSTANCE current, HINSTANCE previous, const std::string & name );
		///< Constructor. Applications should declare
		///< a ApplicationBase object on the stack within
		///< WinMain(), and then call its createWindow() and
		///< run() member functions. The 'name' parameter
		///< is used as the window-class name and the 
		///< title, unless title() and className() are
		///< overridden in the derived class.

	virtual ~ApplicationBase() ;
		///< Virtual destructor.

	void createWindow( int show , bool do_show = true ) ;
		///< Initialisation (was init()). Creates the main
		///< window, etc. Should be called from WinMain().

	void run( bool with_idle = true ) ;
		///< Runs the GetMessage()/DispatchMessage() message pump.
		///< This should be called from WinMain().

	void close() const ;
		///< Sends a close message to this application's
		///< main window, resulting in onClose() being
		///< called.

	virtual std::string title() const ;
		///< Overridable. Defines the main window's title.

	bool messageBoxQuery( const std::string & message ) ; // not const
		///< Puts up a questioning message box.

	void messageBox( const std::string & message ) ; // not const
		///< Puts up a message box.

	static void messageBox( const std::string & title , const std::string & message ) ;
		///< Puts up a message box in the absence of a running application
		///< object.

	virtual UINT resource() const ;
		///< Overridable. Defines the resource id
		///< for the main window's icon and menu.

protected:
	bool firstInstance() const ;
		///< Returns true if the constructor's 'previous' 
		///< parameter was NULL.

	virtual void beep() const ;
		///< Calls ::MessageBeep().
		///<
		///< Overridable as a simple way to keep an
		///< application silent or change the type of
		///< beep.

	virtual std::string className() const ;
		///< Overridable. Defines the main window's class
		///< name.

	virtual HBRUSH backgroundBrush() ;
		///< Overridable. Defines the main window class background brush.
		///< Overrides are typically implemented as
		///< "return (HBRUSH)(1+COLOR_...)".

	virtual DWORD windowStyle() const ;
		///< Overridable. Defines the main window's style.

	virtual DWORD classStyle() const ;
		///< Overridable. Defines the main window class style.

	virtual void onDestroy() ;
		///< Inherited from GGui::Window. Calls PostQuitMessage()
		///< so that the task terminates when its main
		///< window is destroyed.

	virtual void initFirst() ;
		///< Called from init() for the first application
		///< instance. Registers the main window class.
		///< If resource() returns non-zero then it is used
		///< as the icon id.
		///<
		///< May be overridden only if this base class
		///< implementation is called first.

private:
	ApplicationBase( const ApplicationBase & other ) ; // not implemented
	void operator=( const ApplicationBase & other ) ; // not implemented
	static bool messageBoxCore( HWND , unsigned int , 
		const std::string & , const std::string & ) ;
	HWND messageBoxHandle() const ;
	static unsigned int messageBoxType(HWND,unsigned int) ;

private:
	std::string m_name ;
	HINSTANCE m_previous ;
} ;

#endif

