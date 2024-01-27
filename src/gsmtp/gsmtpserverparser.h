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
/// \file gsmtpserverparser.h
///

#ifndef G_SMTP_SERVER_PARSER_H
#define G_SMTP_SERVER_PARSER_H

#include "gdef.h"
#include "gstringview.h"
#include <string>

namespace GSmtp
{
	class ServerParser ;
}

//| \class GSmtp::ServerParser
/// A static mix-in class for GSmtp::ServerProtocol to do SMTP command
/// parsing. Also provides mailboxStyle() to check a mailbox's
/// character-set.
///
/// See also RFC-5321 4.1.2.
///
class GSmtp::ServerParser
{
public:
	enum class MailboxStyle
	{
		Invalid , // eg. NUL or CRLF
		Ascii , // printable ASCII (RFC-5321 4.1.2)
		Utf8 // other (RFC-6531)
	} ;
	struct AddressCommand /// mail-from or rcpt-to
	{
		AddressCommand() = default ;
		AddressCommand( const std::string & e ) : error(e) {}
		std::string error ;
		std::string address ; // SMTP form, possibly quoted and escaped
		bool utf8address {false} ; // ServerParser::mailboxStyle()
		std::size_t tailpos {std::string::npos} ;
		std::size_t size {0U} ;
		std::string auth ;
		std::string body ; // 7BIT, 8BITMIME, BINARYMIME
		bool smtputf8 {false} ; // SMTPUTF8 option
	} ;

	static MailboxStyle mailboxStyle( const std::string & mailbox ) ;
		///< Classifies the given mailbox name.
		///< See also RFC-5198.

	static AddressCommand parseMailFrom( G::string_view ) ;
		///< Parses a MAIL-FROM command.

	static AddressCommand parseRcptTo( G::string_view ) ;
		///< Parses a RCPT-TO command.

	static std::pair<std::size_t,bool> parseBdatSize( G::string_view ) ;
		///< Parses a BDAT command.

	static std::pair<bool,bool> parseBdatLast( G::string_view ) ;
		///< Parses a BDAT LAST command.

	static std::string parseHeloPeerName( const std::string & ) ;
		///< Parses the peer name from an HELO/EHLO command.

	static std::string parseVrfy( const std::string & ) ;
		///< Parses a VRFY command.

private:
	enum class Conversion
	{
		None ,
		ValidXtext ,
		Upper
	} ;
	static AddressCommand parseAddressPart( G::string_view ) ;
	static std::size_t parseMailNumericValue( G::string_view , G::string_view , AddressCommand & ) ;
	static std::string parseMailStringValue( G::string_view , G::string_view , AddressCommand & , Conversion = Conversion::None ) ;
	static bool parseMailBoolean( G::string_view , G::string_view , AddressCommand & ) ;
} ;

#endif
