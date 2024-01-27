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
/// \file gsocks.h
///

#ifndef G_NET_SOCKS_H
#define G_NET_SOCKS_H

#include "gdef.h"
#include "greadwrite.h"
#include "glocation.h"
#include "gexception.h"
#include <string>

namespace GNet
{
	class Socks ;
}

//| \class GNet::Socks
/// Implements the SOCKS4a proxy connection protocol.
///
class GNet::Socks
{
public:
	G_EXCEPTION( SocksError , tx("socks error") ) ;

	explicit Socks( const Location & ) ;
		///< Constructor.

	bool send( G::ReadWrite & ) ;
		///< Sends the connect-request pdu using the given
		///< file descriptor. Returns true if fully sent.

	bool read( G::ReadWrite & ) ;
		///< Reads the response using the given file descriptor.
		///< Returns true if fully received and positive.
		///< Throws if the response is negative.

	static std::string buildPdu( const std::string & far_host , unsigned int far_port ) ;
		///< Builds a SOCKS4a connect request pdu.

private:
	std::size_t m_request_offset {0U} ;
	std::string m_request ;
	std::string m_response ;
} ;

#endif
