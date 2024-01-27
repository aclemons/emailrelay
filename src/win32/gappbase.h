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
/// \file gappbase.h
///

#ifndef G_GUI_APPBASE_H
#define G_GUI_APPBASE_H

#include "gdef.h"
#include "gwindow.h"
#include "gappinst.h"
#include "gexception.h"

namespace GGui
{
	class ApplicationBase ;
}

//| \class GGui::ApplicationBase
/// The ApplicationBase class is a convenient GGui::Window for
/// the application's main window.
///
/// It is initialised by calling createWindow() from WinMain().
/// This registers a Windows window class pointing at the the
/// GGui::Window window procedure (see also GGui::Cracker).
///
/// GGui::ApplicationBase derives from GGui::Window allowing the
/// user to override the default message handing for the main
/// application window.
///
/// \code
/// struct Application : GGui::ApplicationBase
/// {
///   UINT resource() const override { return ID_APP ; } // menu and icon in .rc
///   std::string className() const override { return "MyApp" ; }
///   std::pair<DWORD,DWORD> windowStyle() const override { return Window::windowStyleMain() ; }
///   DWORD classStyle() const override { return Window::classStyle() | CS_... ; }
/// } ;
/// WinMain( hinstance , hprevious , ...)
/// {
///   Application app( hinstance , hprevious , "Test" ) ;
///   app.createWindow() ;
///   app.run() ;
/// }
/// \endcode
///
class GGui::ApplicationBase : public ApplicationInstance , public Window
{
public:
	G_EXCEPTION( RegisterError, tx("cannot register application's window class") ) ;
	G_EXCEPTION( CreateError , tx("cannot create application window") ) ;

	ApplicationBase( HINSTANCE current, HINSTANCE previous, const std::string & name );
		///< Constructor. Applications should instantiate a ApplicationBase-derived
		///< object on the stack within WinMain(), and then call its createWindow()
		///< and run() member functions. The 'name' parameter is used as the
		///< window-class name and the title, unless title() and className()
		///< are overridden in the derived class.

	virtual ~ApplicationBase() ;
		///< Virtual destructor.

	std::string createWindow( int show , bool do_show = true , int dx = 0 , int dy = 0 , bool no_throw = false ) ;
		///< Initialisation. Creates the main window, etc.
		///< This should be called from WinMain().
		///< Returns an error string if no-throw.

	void run() ;
		///< Runs the GGui::Pump class's GetMessage()/DispatchMessage() message
		///< pump. This is typically used by simple GUI applications that do not
		///< have a separate network event loop.

	void close() const ;
		///< Sends a close message to this application's main window, resulting
		///< in onClose() being called.

	virtual std::string title() const ;
		///< Overridable. Defines the main window's title.

	bool messageBoxQuery( const std::string & message ) ;
		///< Puts up a questioning message box.

	void messageBox( const std::string & message ) ;
		///< Puts up a message box.

	static void messageBox( const std::string & title , const std::string & message ) ;
		///< Puts up a message box in the absence of a running application
		///< object.

	virtual UINT resource() const ;
		///< Overridable. Defines the resource id for the main window's
		///< icon and menu.

protected:
	bool firstInstance() const ;
		///< Returns true if the constructor's 'previous'
		///< parameter was null.

	virtual void beep() const ;
		///< Calls MessageBeep().
		///<
		///< Overridable as a simple way to keep an application
		///< silent or change the type of beep.

	virtual std::string className() const ;
		///< Overridable. Defines the main window's class name.

	virtual HBRUSH backgroundBrush() ;
		///< Overridable. Defines the main window class background
		///< brush. Overrides are typically implemented as
		///< "return (HBRUSH)(1+COLOR_...)".

	virtual std::pair<DWORD,DWORD> windowStyle() const ;
		///< Overridable. Defines the main window's style and
		///< the CreateWindowEx extended style.

	virtual DWORD classStyle() const ;
		///< Overridable. Defines the main window class style.

	virtual void onDestroy() override ;
		///< Override from GGui::Window. Calls GGui::Pump::quit().

	virtual void initFirst() ;
		///< Called from init() for the first application instance.
		///< Registers the main window class. If resource() returns
		///< non-zero then it is used as the icon id.
		///<
		///< If overridden then this base class implementation must
		///< be called first.

public:
	ApplicationBase( const ApplicationBase & ) = delete ;
	ApplicationBase( ApplicationBase && ) = delete ;
	ApplicationBase & operator=( const ApplicationBase & ) = delete ;
	ApplicationBase & operator=( ApplicationBase && ) = delete ;

private:
	static bool messageBoxCore( HWND , unsigned int , const std::string & , const std::string & ) ;
	HWND messageBoxHandle() const ;
	static unsigned int messageBoxType(HWND,unsigned int) ;

private:
	std::string m_name ;
	HINSTANCE m_previous ;
} ;

#endif
