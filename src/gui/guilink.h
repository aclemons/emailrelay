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
/// \file guilink.h
///

#ifndef GUI_LINK_H
#define GUI_LINK_H

#include "gdef.h"
#include "gpath.h"
#include "gexception.h"
#include "gstringarray.h"
#include <string>

namespace Gui
{
	class Link ;
	class LinkImp ;
}

//| \class Gui::Link
/// A class for creating desktop links (aka "shortcuts") and
/// application menu items.
///
class Gui::Link
{
public:
	G_EXCEPTION( SaveError , tx("error saving desktop or menu link") ) ;

	enum class Show { Default , Hide } ;

	Link( const G::Path & target_path , const std::string & name , const std::string & description ,
		const G::Path & working_dir , const G::StringArray & args = G::StringArray() ,
		const G::Path & icon_source = G::Path() , Show show = Show::Default ,
		const std::string & internal_comment_1 = {} ,
		const std::string & internal_comment_2 = {} ,
		const std::string & internal_comment_3 = {} ) ;
			///< Constructor. Note that the path of the link itself
			///< is specified in saveAs(), not the constructor.
			///< The "working_dir" is the current-working-directory
			///< when the link is used.

	static std::string filename( const std::string & name ) ;
		///< Returns a normalised filename including an extension like ".lnk" or ".desktop".

	void saveAs( const G::Path & link_path ) ;
		///< Saves the link.

	~Link() ;
		///< Destructor.

	static bool remove( const G::Path & link_path ) ;
		///< Removes a link. Returns true if removed.

	static bool exists( const G::Path & link_path ) ;
		///< Returns true if the link exists.

	static bool exists( const G::Path & dir , const std::string & link_name ) ;
		///< Returns true if the link exists.

public:
	Link( const Link & ) = delete ;
	Link( Link && ) = delete ;
	Link & operator=( const Link & ) = delete ;
	Link & operator=( Link && ) = delete ;

private:
	std::unique_ptr<LinkImp> m_imp ;
} ;

inline
bool Gui::Link::exists( const G::Path & dir , const std::string & link_name )
{
	return !dir.empty() && !link_name.empty() && exists( dir + link_name ) ;
}

#endif
