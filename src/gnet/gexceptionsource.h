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
/// \file gexceptionsource.h
///

#ifndef G_NET_EXCEPTION_SOURCE_H
#define G_NET_EXCEPTION_SOURCE_H

#include "gdef.h"

namespace GNet
{
	class ExceptionSource ;
}

//| \class GNet::ExceptionSource
/// A mixin base class that identifies the source of an exception
/// when delivered to GNet::ExceptionHandler and optionally provides
/// an indentifier for logging purposes.
///
/// The primary motivation is to allow a Server to manage its
/// ServerPeer list when one of them throws an exception.
///
class GNet::ExceptionSource
{
public:
	virtual std::string exceptionSourceId() const ;
		///< Returns an identifying string for logging purposes,
		///< or the empty string. This typically provides the
		///< remote peer's network address. The default
		///< implementation returns the empty string.

	virtual ~ExceptionSource() ;
		///< Destructor.

public:
	ExceptionSource() = default ;
	ExceptionSource( const ExceptionSource & ) = delete ;
	ExceptionSource( ExceptionSource && ) = delete ;
	ExceptionSource & operator=( const ExceptionSource & ) = delete ;
	ExceptionSource & operator=( ExceptionSource && ) = delete ;
} ;

#endif
