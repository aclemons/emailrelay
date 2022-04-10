//
// Copyright (C) 2001-2021 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gfactoryparser.h
///

#ifndef G_SMTP_FACTORY_PARSER_H
#define G_SMTP_FACTORY_PARSER_H

#include "gdef.h"
#include <utility>
#include <string>

namespace GSmtp
{
	class FactoryParser ;
}

//| \class GSmtp::FactoryParser
/// A simple static class to parse identifiers that are either a
/// program path or a network address. Used by the filter factory
/// and the address-verifier factory.
///
class GSmtp::FactoryParser
{
public:
	struct Result /// Result tuple for GSmtp::FactoryParser::parse().
	{
		Result() ;
		Result( const std::string & , const std::string & ) ;
		Result( const std::string & , const std::string & , int ) ;
		std::string first ; // eg. "file", "net", "spam"
		std::string second ; // eg. "localhost:99"
		int third{0} ; // eg. 1 for spam-edit
	} ;

	static Result parse( const std::string & identifier , bool allow_spam ) ;
		///< Parses an identifier like "/usr/bin/foo" or "net:127.0.0.1:99"
		///< or "net:/run/spamd.s", returning the type and the specification
		///< in a result tuple, eg. ("file","/usr/bin/foo") or
		///< ("net","127.0.0.1:99"). Returns a default-constructed Result
		///< if not parsable.

	static std::string check( const std::string & identifier , bool allow_spam ) ;
		///< Parses and checks an identifier. Returns a diagnostic if
		///< the identifier is invalid, or the empty string if valid
		///< or empty.

public:
	FactoryParser() = delete ;
} ;

#endif
