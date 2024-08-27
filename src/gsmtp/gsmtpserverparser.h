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
/// \file gsmtpserverparser.h
///

#ifndef G_SMTP_SERVER_PARSER_H
#define G_SMTP_SERVER_PARSER_H

#include "gdef.h"
#include "gstringview.h"
#include "gmessagestore.h"
#include <string>

namespace GSmtp
{
	class ServerParser ;
}

//| \class GSmtp::ServerParser
/// A static class for SMTP command parsing, used by GSmtp::ServerProtocol
/// as a mix-in base.
///
/// See also RFC-5321 4.1.2.
///
class GSmtp::ServerParser
{
public:
	struct Config /// A configuration structure for GSmtp::ServerParser.
	{
		bool allow_spaces {false} ; // sr #89
		bool allow_nobrackets {false} ; // sr #97
		bool alabels {false} ; // normalise domain names using A-labels
		std::string allow_spaces_help ;
		std::string allow_nobrackets_help ;
		Config & set_allow_spaces( bool b = true ) noexcept ;
		Config & set_allow_nobrackets( bool b = true ) noexcept ;
		Config & set_alabels( bool b = true ) noexcept ;
		Config & set_allow_spaces_help( const std::string & ) ;
		Config & set_allow_nobrackets_help( const std::string & ) ;
	} ;
	using AddressStyle = GStore::MessageStore::AddressStyle ;
	struct AddressCommand /// mail-from or rcpt-to
	{
		AddressCommand() = default ;
		AddressCommand( const std::string & e ) : error(e) {}
		std::string error ;
		std::string raw_address ; // raw address, possibly UTF-8 and/or with local-part quoted and escaped
		std::string address ; // address with domain part using A-labels (if requested by Config::alabels)
		AddressStyle address_style {AddressStyle::Ascii} ; // see GStore::MessageStore::addressStyle()
		bool utf8_mailbox_part {false} ; // see address_style
		bool utf8_domain_part {false} ; // see address_style
		std::size_t tailpos {std::string::npos} ;
		std::size_t size {0U} ;
		std::string auth ;
		std::string body ; // 7BIT, 8BITMIME, BINARYMIME
		bool smtputf8 {false} ; // SMTPUTF8 option
		bool invalid_spaces {false} ;
		bool invalid_nobrackets {false} ;
	} ;

	static AddressCommand parseMailFrom( std::string_view , const Config & ) ;
		///< Parses a MAIL-FROM command.

	static AddressCommand parseRcptTo( std::string_view , const Config & ) ;
		///< Parses a RCPT-TO command.

	static std::pair<std::size_t,bool> parseBdatSize( std::string_view ) ;
		///< Parses a BDAT command.

	static std::pair<bool,bool> parseBdatLast( std::string_view ) ;
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
	static AddressCommand parseAddressPart( std::string_view , const Config & ) ;
	static std::size_t parseMailNumericValue( std::string_view , std::string_view , AddressCommand & ) ;
	static std::string parseMailStringValue( std::string_view , std::string_view , AddressCommand & , Conversion = Conversion::None ) ;
	static bool parseMailBoolean( std::string_view , std::string_view , AddressCommand & ) ;
	static std::string encodeDomain( std::string_view ) ;
} ;

inline GSmtp::ServerParser::Config & GSmtp::ServerParser::Config::set_allow_spaces( bool b ) noexcept { allow_spaces = b ; return *this ; }
inline GSmtp::ServerParser::Config & GSmtp::ServerParser::Config::set_allow_nobrackets( bool b ) noexcept { allow_nobrackets = b ; return *this ; }
inline GSmtp::ServerParser::Config & GSmtp::ServerParser::Config::set_alabels( bool b ) noexcept { alabels = b ; return *this ; }
inline GSmtp::ServerParser::Config & GSmtp::ServerParser::Config::set_allow_spaces_help( const std::string & s ) { allow_spaces_help = s ; return *this ; }
inline GSmtp::ServerParser::Config & GSmtp::ServerParser::Config::set_allow_nobrackets_help( const std::string & s ) { allow_nobrackets_help = s ; return *this ; }

#endif
