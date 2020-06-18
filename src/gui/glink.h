//
// Copyright (C) 2001-2020 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file glink.h
///

#ifndef G_LINK_H
#define G_LINK_H

#include "gdef.h"
#include "gpath.h"
#include "gexception.h"
#include "gstrings.h"
#include <string>

class GLinkImp ;

/// \class GLink
/// A class for creating desktop links (aka "shortcuts") and
/// application menu items.
///
class GLink
{
public:
	G_EXCEPTION( SaveError , "error saving desktop or menu link" ) ;

	enum class Show { Default , Hide } ;

	GLink( const G::Path & target_path , const std::string & name , const std::string & description ,
		const G::Path & working_dir , const G::StringArray & args = G::StringArray() ,
		const G::Path & icon_source = G::Path() , Show show = Show::Default ,
		const std::string & internal_comment_1 = std::string() ,
		const std::string & internal_comment_2 = std::string() ,
		const std::string & internal_comment_3 = std::string() ) ;
			///< Constructor. Note that the path of the link itself
			///< is specified in saveAs(), not the constructor.
			///< The "working_dir" is the current-working-directory
			///< when the link is used.

	static std::string filename( const std::string & name ) ;
		///< Returns a normalised filename including an extension like ".lnk" or ".desktop".

	void saveAs( const G::Path & link_path ) ;
		///< Saves the link.

	~GLink() ;
		///< Destructor.

	static bool remove( const G::Path & link_path ) ;
		///< Removes a link. Returns true if removed.

	static bool exists( const G::Path & link_path ) ;
		///< Returns true if the link exists.

public:
	GLink( const GLink & ) = delete ;
	GLink( GLink && ) = delete ;
	void operator=( const GLink & ) = delete ;
	void operator=( GLink && ) = delete ;

private:
	std::unique_ptr<GLinkImp> m_imp ;
} ;

#endif
