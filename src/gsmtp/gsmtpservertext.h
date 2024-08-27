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
/// \file gsmtpservertext.h
///

#ifndef G_SMTP_SERVER_TEXT_H
#define G_SMTP_SERVER_TEXT_H

#include "gdef.h"
#include "gsmtpserverprotocol.h"
#include "gaddress.h"
#include <string>

namespace GSmtp
{
	class ServerText ;
}

//| \class GSmtp::ServerText
/// A default implementation of the GSmtp::ServerProtocol::Text interface.
///
class GSmtp::ServerText : public GSmtp::ServerProtocol::Text
{
public:
	ServerText( const std::string & code_ident , bool anonymous , bool with_received_line ,
		const std::string & greeting_and_receivedline_domain , const GNet::Address & peer_address ) ;
			///< Constructor.

	static std::string receivedLine( const std::string & smtp_peer_name ,
		const std::string & peer_address , const std::string & receivedline_domain ,
		bool authenticated , bool secure , const std::string & , const std::string & cipher_in ) ;

public:
	~ServerText() override = default ;
	ServerText( const ServerText & ) = default ;
	ServerText( ServerText && ) = default ;
	ServerText & operator=( const ServerText & ) = default ;
	ServerText & operator=( ServerText && ) = default ;

private: // overrides:
	std::string greeting() const override ; // Override from GSmtp::ServerProtocol::Text.
	std::string hello( const std::string & smtp_peer_name_from_helo ) const override ; // Override from GSmtp::ServerProtocol::Text.
	std::string received( const std::string & , bool , bool , const std::string & , const std::string & ) const override ; // Override from GSmtp::ServerProtocol::Text.

private:
	std::string m_code_ident ;
	bool m_anonymous ;
	bool m_with_received_line ;
	std::string m_domain ; // greeting and receivedline
	GNet::Address m_peer_address ;
} ;

#endif
