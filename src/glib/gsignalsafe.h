//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gsignalsafe.h
///

#ifndef G_SIGNAL_SAFE_H
#define G_SIGNAL_SAFE_H

#include "gdef.h"

namespace G
{
	class SignalSafe ;
}

//| \class G::SignalSafe
/// An empty structure that is used to indicate a signal-safe, reentrant implementation.
/// Any function with SignalSafe in its signature should be kept small and simple,
/// with no exceptions, no logging and no blocking system calls.
///
class G::SignalSafe
{
} ;

#endif
