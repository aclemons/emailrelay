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
/// \file glistener.h
///

#ifndef G_NET_LISTENER_H
#define G_NET_LISTENER_H

#include "gdef.h"
#include "gaddress.h"

namespace GNet
{
	class Listener ;
}

//| \class GNet::Listener
/// An interface for a network listener.
/// \see GNet::Server, GNet::Monitor
///
class GNet::Listener
{
public:
	virtual ~Listener() = default ;
		///< Destructor.

	virtual Address address() const = 0 ;
		///< Returns the listening address.
} ;

#endif
