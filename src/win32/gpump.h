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
/// \file gpump.h
///

#ifndef G_PUMP_H
#define G_PUMP_H

#include "gdef.h"
#include <string>

/// \namespace GGui
namespace GGui
{
	class Pump ;
}

/// \class GGui::Pump
/// A static class which implements a Windows message pump. 
/// Uses GGui::Dialog::dialogMessage() in its implementation in order to 
/// support modeless dialog boxes.
///
/// \see GGui::Cracker, GGui::Dialog, GGui::ApplicationInstance
///
class GGui::Pump 
{
public:
	static std::string run() ;
		///< GetMessage()/DispatchMessage() message pump.
		///< Typically called from WinMain(). Returns a
		///< reason string if quit() was called.

	static std::string run( HWND idle_window , unsigned int idle_message ) ;
		///< An overload which sends idle messages once
		///< the message queue is empty. If the idle message 
		///< handler returns 1 then the message is sent again.

	static void quit( std::string reason = std::string() ) ;
		///< Causes run() to return (once the call stack
		///< has unwound). Use this in preference to
		///< ::PostQuitMessage().

private:
	static bool dialogMessage( MSG & ) ; // links gpump.cpp to gdialog.cpp (or not)
	Pump() ; // not implemented
	static bool empty() ;
	static bool sendIdle( HWND , unsigned int ) ;
	static std::string runCore( bool , HWND , unsigned int ) ;
} ;

#endif
