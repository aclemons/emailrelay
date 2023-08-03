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
/// \file gfilterfactory.h
///

#ifndef G_FILTER_FACTORY_H
#define G_FILTER_FACTORY_H

#include "gdef.h"
#include "gfilterfactorybase.h"
#include "gfilestore.h"
#include "gpath.h"
#include <string>
#include <utility>
#include <memory>
#include <cstddef> // std::nullptr_t

namespace GFilters
{
	class FilterFactory ;
}

//| \class GFilters::FilterFactory
/// A FilterFactory implementation. It holds a GSmtp::FileStore reference
/// so that it can instantiate filters that operate messages stored as
/// files.
///
class GFilters::FilterFactory : public GSmtp::FilterFactoryBase
{
public:
	explicit FilterFactory( GStore::FileStore & ) ;
		///< Constructor. The FileStore reference is retained and passed
		///< to new filter objects so that they can derive the paths of
		///< the content and envelope files that they process.

	static Spec parse( const std::string & spec , const G::Path & base_dir = {} ,
		const G::Path & app_dir = {} , G::StringArray * warnings_p = nullptr ) ;
			///< Parses and validates the filter specification string returning
			///< the type and value in a Spec tuple, eg. ("file","/usr/bin/foo")
			///< or ("net","127.0.0.1:99").
			///<
			///< The specification string can be a comma-separated list with
			///< the component parts checked separately and the returned Spec
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

protected: // overrides
	std::unique_ptr<GSmtp::Filter> newFilter( GNet::ExceptionSink ,
		GSmtp::Filter::Type , const GSmtp::Filter::Config & ,
		const Spec & ) override ;

private:
	static void checkNumber( Spec & ) ;
	static void checkNet( Spec & ) ;
	static void checkRange( Spec & ) ;
	static void checkFile( Spec & , G::StringArray * ) ;
	static void fixFile( Spec & , const G::Path & , const G::Path & ) ;

private:
	GStore::FileStore & m_file_store ;
} ;

#endif
