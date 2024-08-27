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
/// \file gpop.h
///

#ifndef G_POP_H
#define G_POP_H

#include "gdef.h"
#include "gpopstore.h"
#include "gpopserver.h"
#include "gsecrets.h"
#include "geventstate.h"
#include "gpath.h"
#include <memory>

namespace GPop
{
	bool enabled() noexcept ;
		///< Returns true if pop code is built in.

	std::unique_ptr<Store> newStore( const G::Path & spool_dir , const Store::Config & ) ;
		///< Creates a new Pop::Store.

	std::unique_ptr<GAuth::SaslServerSecrets> newSecrets( const std::string & path ) ;
		///< Creates a new SaslServerSecrets for newStore().

	std::unique_ptr<Server> newServer( GNet::EventState , Store & ,
		const GAuth::SaslServerSecrets & , const Server::Config & ) ;
			///< Creates a new server.

	void report( const Server * , const std::string & group = {} ) ;
		///< Calls GPop::Server::report().
}

#endif

