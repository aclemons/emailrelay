//
// Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gconnectiontable.h
///

#ifndef G_CONNECTION_TABLE_H 
#define G_CONNECTION_TABLE_H 

#include "gdef.h"
#include "gnet.h"
#include "gaddress.h"
#include <string>

/// \namespace GNet
namespace GNet
{
	class ConnectionTable ;
	class ConnectionTableImp ;
}

/// \class GNet::ConnectionTable
/// A class for getting more information about a connection.
///
class GNet::ConnectionTable 
{
public:
	/// Holds information provided by GNet::ConnectionTable::find().
	struct Connection 
	{
		bool valid() const ;
		std::string peerName() const ;
		std::string m_peer_name ;
		bool m_valid ;
	} ;

	ConnectionTable() ;
		///< Constructor.

	~ConnectionTable() ;
		///< Destructor.

	Connection find( Address local , Address peer ) ;
		///< Looks up the connection. Returns an invalid
		///< Connection on error.

private:
	ConnectionTable( const ConnectionTable & ) ;
	void operator=( const ConnectionTable & ) ;

private:
	ConnectionTableImp * m_imp ;
} ;

#endif
