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
/// \file gvalid.h
///

#ifndef G_VALID_H
#define G_VALID_H

#include "gdef.h"

namespace GAuth
{
	class Valid ;
}

//| \class GAuth::Valid
/// A trivial mix-in interface containing a valid() method.
///
class GAuth::Valid
{
public:
	virtual bool valid() const = 0 ;
		///< Returns true if a valid source of information.

	virtual ~Valid() = default ;
		///< Destructor.
} ;

#endif
