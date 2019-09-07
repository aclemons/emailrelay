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
/// \file gfactoryparser.h
///

#ifndef G_SMTP_FACTORY_PARSER__H
#define G_SMTP_FACTORY_PARSER__H

#include "gdef.h"
#include <utility>
#include <string>

namespace GSmtp
{
	class FactoryParser ;
}

/// \class GSmtp::FactoryParser
/// A simple static class to parse identifiers that can be a
/// program in the file system or a network address. Used by
/// the filter factory and the address-verifier factory.
///
class GSmtp::FactoryParser
{
public:
	struct Result /// Result tuple for GSmtp::FactoryParser::parse().
	{
		Result() ;
		Result( const std::string & , const std::string & ) ;
		Result( const std::string & , const std::string & , int ) ;
		std::string first ;
		std::string second ;
		int third ;
	} ;

	static Result parse( const std::string & identifier , bool allow_spam ) ;
		///< Parses an identifier like "/usr/bin/foo" or "net:127.0.0.1:99"
		///< returning the type and the specification in a result tuple, eg.
		///< ("file","/usr/bin/foo") or ("net","127.0.0.1:99").

	static std::string check( const std::string & identifier , bool allow_spam ) ;
		///< Parses and checks an identifier. Returns a diagnostic if
		///< the identifier is invalid, or the empty string if valid
		///< or empty.

private:
	FactoryParser() g__eq_delete ;
} ;

#endif
