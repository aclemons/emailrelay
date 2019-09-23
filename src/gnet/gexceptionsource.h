//
// Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gexceptionsource.h
///

#ifndef G_NET_EXCEPTION_SOURCE__H
#define G_NET_EXCEPTION_SOURCE__H

#include "gdef.h"

namespace GNet
{
	class ExceptionSource ;
}

/// \class GNet::ExceptionSource
/// An empty class that identifies the source of an exception as
/// delivered to a GNet::ExceptionHandler interface. This is used
/// as a public mixin base class for classes that are held in a
/// container and need to be identified from within the onException()
/// call delivered to the container.
///
class GNet::ExceptionSource
{
public:
	virtual ~ExceptionSource() ;
		///< Destructor.
} ;

#endif
