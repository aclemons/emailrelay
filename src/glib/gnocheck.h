//
// Copyright (C) 2001-2010 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gnocheck.h
///

#ifndef G_NO_CHECK_H__
#define G_NO_CHECK_H__

#include "gdef.h"
#include "gnoncopyable.h"

/// \namespace G
namespace G
{
	class NoCheck ;
	class NoCheckImp ;
}

/// \class G::NoCheck
/// A class which affects the run-time
/// library's invalid-parameter checking behaviour
/// while in scope.
///
class G::NoCheck : private G::noncopyable 
{
public:
	NoCheck() ;
		///< Default constructor. Turns off run-time 
		///< library parameter checking.

	~NoCheck() ;
		///< Desctructor. Restores parameter checking.

private:
	NoCheckImp * m_imp ;
} ;

#endif

