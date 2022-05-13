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
/// \file glocal.h
///

#ifndef G_NET_LOCAL_H
#define G_NET_LOCAL_H

#include "gdef.h"
#include "gaddress.h"
#include "glocation.h"
#include "gexception.h"

namespace GNet
{
	class Local ;
}

//| \class GNet::Local
/// A static class for getting information about the local machine's network
/// name and address.
///
class GNet::Local
{
public:
	static std::string hostname() ;
		///< Returns the local hostname. Returns "localhost" on error.

	static std::string canonicalName() ;
		///< Returns the canonical network name assiciated with hostname().
		///< Defaults to "<hostname>.localnet" if DNS does not provide
		///< a canonical network name.

	static void canonicalName( const std::string & override ) ;
		///< Sets the canonicalName() override.

	static bool isLocal( const Address & , std::string & reason ) ;
		///< Returns true if the given address appears to be 'local',
		///< or a helpful error message if not.

	static bool isLocal( const Address & ) ;
		///< Overload without an explanation.

public:
	Local() = delete ;

private:
	static std::string resolvedHostname() ;

private:
	static std::string m_name_override ;
	static bool m_name_override_set ;
} ;

#endif
