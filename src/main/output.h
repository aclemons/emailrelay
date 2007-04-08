//
// Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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
///
/// \file output.h
///

#ifndef G_MAIN_OUTPUT_H
#define G_MAIN_OUTPUT_H

#include "gdef.h"
#include <string>

/// \namespace Main
namespace Main
{
	class Output ;
}

/// \class Main::Output
/// An abstract interface for generating output.
/// The implementation is controlled from main()/WinMain().
///
class Main::Output 
{
public: 
	virtual unsigned int columns() = 0 ;
		///< Returns the output line width.

	virtual void output( const std::string & , bool error ) = 0 ;
		///< Outputs the given string.

	virtual ~Output() ;
		///< Destructor.

private:
	void operator=( const Output & ) ; // not implemented
} ;

#endif

