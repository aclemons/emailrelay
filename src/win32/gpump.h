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
/// \file gpump.h
///

#ifndef G_GUI_PUMP_H
#define G_GUI_PUMP_H

#include "gdef.h"
#include <string>
#include <utility>

namespace GGui
{
	class Pump ;
}

//| \class GGui::Pump
/// A static class which implements a Windows GetMessage/DispatchMessage
/// message pump. While the pump is run()ning it pulls messages out
/// of the message queue and dispatches them to the relevant window
/// procedure.
///
/// Uses GGui::Dialog::dialogMessage() in its implementation in order
/// to support modeless dialog boxes.
///
/// The implementation guarantees that there will be no extraneous
/// calls to PeekMessage() that might upset MsgWaitForMultipleObjects().
///
/// Optionally idle messages can be generated when the message queue
/// becomes empty.
///
/// \see GGui::Cracker, GGui::Dialog, GGui::ApplicationInstance
///
class GGui::Pump
{
public:
	static std::string run() ;
		///< Runs the GetMessage()/DispatchMessage() message pump.
		///< Returns a reason string if quit() was called.

	static std::string run( HWND idle_window , unsigned int idle_message ) ;
		///< An overload that sends an idle message whenever the
		///< message queue gets to empty. The idle message handler
		///< should return 0 if it has more idle work to do, or 1
		///< if complete.

	static std::pair<bool,std::string> runToEmpty() ;
		///< Runs the PeekMessage()/DispatchMessage() message pump
		///< until the message queue gets to empty. Returns true and
		///< a reason string if quit() was called at some point.

	static std::pair<bool,std::string> runToEmpty( HWND idle_window , unsigned int idle_message ) ;
		///< An overload that sends an idle message when the
		///< message queue gets to empty for the first time
		///< and then returns if-and-when the message queue is
		///< empty. If the idle message handler returns 0
		///< then multiple idle message can be sent.

	static void quit( const std::string & reason = {} ) ;
		///< Causes run() to return as soon as the call stack
		///< has unwound, or sets the return value for
		///< runToEmpty().

public:
	Pump() = delete ;

private:
	static bool getMessage( MSG * , bool ) ;
	static bool empty() ;
	static bool sendIdle( HWND , unsigned int ) ;
	static std::pair<bool,std::string> runImp( bool , HWND , unsigned int , bool ) ;
	static WPARAM m_run_id ;
	static std::string m_quit_reason ;
} ;

#endif
