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
/// \file gfactoryparser.h
///

#ifndef G_SMTP_FACTORY_PARSER_H
#define G_SMTP_FACTORY_PARSER_H

#include "gdef.h"
#include "ggettext.h"
#include "gpath.h"
#include "gexception.h"
#include <utility>
#include <string>

namespace GSmtp
{
	class FactoryParser ;
}

//| \class GSmtp::FactoryParser
/// A simple static class to parse filter specifications that are
/// either a program path or a network address. Used by the filter
/// factory and the address-verifier factory.
///
class GSmtp::FactoryParser
{
public:
	G_EXCEPTION( Error , tx("invalid specification") ) ;

	struct Result /// Result tuple for GSmtp::FactoryParser::parse().
	{
		Result() ;
		Result( const std::string & , const std::string & ) ;
		Result( const std::string & , const std::string & , int ) ;
		std::string first ; // eg. "file", "net", "spam"
		std::string second ; // eg. "localhost:99"
		int third{0} ; // eg. 1 for spam-edit
	} ;

	static Result parse( const std::string & spec , bool allow_spam , bool allow_chain ) ;
		///< Parses a filter specification like "/usr/bin/foo" or
		///< "net:127.0.0.1:99" or "net:/run/spamd.s", returning the
		///< type and value in a result tuple, eg. ("file","/usr/bin/foo")
		///< or ("net","127.0.0.1:99"). Throws if not parsable.

public:
	FactoryParser() = delete ;

private:
	static void checkFile( const G::Path & ) ;
	static std::string checkExit( const std::string & ) ;
} ;

#endif
