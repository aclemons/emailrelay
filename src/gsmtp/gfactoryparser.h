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
#include "gstringarray.h"
#include "gexception.h"
#include <utility>
#include <string>

namespace GSmtp
{
	class FactoryParser ;
}

//| \class GSmtp::FactoryParser
/// A static class to parse filter and verifier specifications that are
/// either a program path or a network address etc. Used by the filter
/// factory and the address-verifier factory classes.
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
		std::string first ; // "exit", "file", "net", "spam", "chain", empty on error
		std::string second ; // reason on error, or eg. "/bin/a" if "file", eg. "file:/bin/a,file:/bin/b" if "chain"
		int third{0} ; // eg. 1 for spam-edit
	} ;

	static Result parse( const std::string & spec , bool is_filter ,
		const G::Path & base_dir = {} , const G::Path & app_dir = {} ,
		G::StringArray * warnings_p = nullptr ) ;
			///< Parses a filter or verifier specification like "/usr/bin/foo" or
			///< "net:127.0.0.1:99" or "net:/run/spamd.s", returning the
			///< type and value in a result tuple, eg. ("file","/usr/bin/foo")
			///< or ("net","127.0.0.1:99").
			///<
			///< If 'is-filter' then the spec can be a comma-separated list with
			///< the component parts checked separately and the returned Result
			///< is like ("chain","file:foo,net:bar").
			///<
			///< Any relative file paths are made absolute using the given
			///< base directory, if given. (This is normally from
			///< G::Process::cwd() called at startup).
			///<
			///< Any "@app" sub-strings in file paths are substituted with
			///< the given application directory, if given.
			///<
			///< Returns 'first' empty if a fatal parsing error, with the
			///< reason in 'second'.
			///<
			///< Returns warnings by reference for non-fatal errors, such
			///< as missing files.

public:
	FactoryParser() = delete ;

private:
	static Result parseImp( const std::string & , bool , const G::Path & , const G::Path & , G::StringArray * , bool , bool ) ;
	static void normalise( Result & , const G::Path & , const G::Path & ) ;
	static void check( Result & , bool , G::StringArray * ) ;
} ;

#endif
