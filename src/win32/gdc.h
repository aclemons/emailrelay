//
// Copyright (C) 2001-2003 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later
// version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
// ===
//
// gdc.h
//

#ifndef G_DC_H
#define G_DC_H

#include "gdef.h"

namespace GGui
{
	class DeviceContext ;
	class ScreenDeviceContext ;
}

// Class: GGui::DeviceContext
// Description: A thin wrapper for a GDI device 
// context corresponding to a window.
// See also: GGui::ScreenDeviceContext
//
class GGui::DeviceContext 
{
public:
	explicit DeviceContext( HWND hwnd ) ;
		// Constructor for a window's device context.
		//
		// The GDI device context is released in
		// the destructor.

	explicit DeviceContext( HDC hdc ) ;
		// Constructor to wrap the given GDI handle.
		// The GDI handle typically comes from ::BeginPaint()
		// while processing a WM_PAINT message.
		//
		// The GDI device context is _not_ released in
		// the destructor.

	~DeviceContext() ;
		// Destructor.

	HDC handle() const ;
		// Returns the GDI device context handle.

	HDC extractHandle() ;
		// Extracts the GDI device context handle.
		// The destructor will no longer release it.

	HDC operator()() const ;
		// Returns the GDI device context handle.

	void swapBuffers() ;
		// If the device context has double buffering
		// then the two pixel buffers are swapped.
		// This is typically called after the 
		// "back" buffer has been filled with
		// a new image.

private:
	HDC m_hdc ;
	HWND m_hwnd ;
	bool m_do_release ;

private:
	DeviceContext( const DeviceContext & ) ;
	void operator=( const DeviceContext & ) ;
} ;

// Class: GGui::ScreenDeviceContext
// Description: A thin wrapper for a GDI device 
// context corresponding to the whole screen.
// See also: GGui::DeviceContext
//
class GGui::ScreenDeviceContext 
{
public:
	ScreenDeviceContext() ;
		// Default constructor.

	~ScreenDeviceContext() ;
		// Destructor.

	HDC handle() ;
		// Returns the GDI device context handle.

	HDC operator()() ;
		// Returns the GDI device context handle.

	int colours() const ;
		// Returns the number of colours.

	int dx() const ;
		// Returns the screen width.

	int dy() const ;
		// Returns the screen height.

	int aspectx() const ;
		// Returns one part of the screen's aspect ratio.

	int aspecty() const ;
		// Returns the other part of the screen's aspect ratio.

private:
	ScreenDeviceContext( const ScreenDeviceContext & ) ;
	void operator=( const ScreenDeviceContext & ) ;

private:
	HDC m_dc ;
} ;

#endif
