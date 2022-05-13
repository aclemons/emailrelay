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
/// \file glink_mac.cpp
///

#include "gdef.h"
#include "gstr.h"
#include "glink.h"
#include "gfile.h"
#include <string>
#include <sstream>
#include <fstream>
#include <cstdlib> // std::system()

class G::LinkImp
{
public:
	LinkImp( const Path & target_path , const std::string & name , const std::string & description ,
		const Path & working_dir , const StringArray & args , const Path & icon_source , Link::Show show ) ;
	static std::string filename( const std::string & ) ;
	void saveAs( const Path & ) ;

public:
	~LinkImp() = default ;
	LinkImp( const LinkImp & ) = delete ;
	LinkImp( LinkImp && ) = delete ;
	void operator=( const LinkImp & ) = delete ;
	void operator=( LinkImp && ) = delete ;

private:
	Path m_target_path ;
	std::string m_name ;
} ;

G::LinkImp::LinkImp( const Path & target_path , const std::string & name , const std::string & ,
	const Path & , const StringArray & , const Path & , Link::Show ) :
		m_target_path(target_path) ,
		m_name(name)
{
}

std::string G::LinkImp::filename( const std::string & )
{
	return std::string() ;
}

void G::LinkImp::saveAs( const Path & )
{
	// TODO -- hardcoded Start.app name
	//
	// we have both a start/stop script and a startup application bundle
	// and here we need to convert one to the other -- should maybe expose
	// the installer's LinkInfo structure at this interface and add a
	// third section to it
	//
	Path start_app_path( m_target_path.dirname() , "E-MailRelay-Start.app" ) ;
	if( !File::exists(start_app_path) )
		start_app_path = Path( m_target_path.dirname() , "../E-MailRelay-Start.app" ) ;

	std::ostringstream ss ;
	ss
		<< "/usr/bin/osascript "
			<< "-e \"tell application \\\"System Events\\\"\" "
			<< "-e \"make new login item at end of login items with properties {"
				<< "path:\\\"" << start_app_path << "\\\","
				<< "hidden:true}\" "
			<< "-e \"end tell\"" ;

	GDEF_IGNORE_RETURN std::system( ss.str().c_str() ) ;
}

// ==

G::Link::Link( const Path & target_path , const std::string & name , const std::string & description ,
	const Path & working_dir , const StringArray & args , const Path & icon_source , Show show ,
	const std::string & , const std::string & , const std::string & ) :
		m_imp( std::make_unique<LinkImp>(target_path,name,description,working_dir,args,icon_source,show) )
{
}

G::Link::~Link()
= default ;

std::string G::Link::filename( const std::string & name_in )
{
	return LinkImp::filename( name_in ) ;
}

void G::Link::saveAs( const Path & link_path )
{
	m_imp->saveAs( link_path ) ;
}

bool G::Link::exists( const Path & path )
{
	return File::exists( path.dirname()+"E-MailRelay-Start.app" , std::nothrow ) ; // not tested
}

bool G::Link::remove( const Path & )
{
	std::ostringstream ss ;
	ss
		<< "/usr/bin/osascript "
			<< "-e \"tell application \\\"System Events\\\"\" "
			<< "-e \"properties of every login item\" "
			<< "-e \"end tell\" | "
		<< "/usr/bin/sed 's/class:/%class:/g' | "
		<< "/usr/bin/tr '%' '\\n' | "
		<< "/usr/bin/grep -F 'class:' | "
		<< "/usr/bin/grep -F -n E-MailRelay | "
		<< "/usr/bin/sed 's/:.*//' | "
		<< "/usr/bin/tail -1 | "
		<< "/usr/bin/xargs -I __ "
			<< "/usr/bin/osascript "
				<< "-e \"tell application \\\"System Events\\\"\" "
				<< "-e \"delete login item __\" "
				<< "-e \"end tell\"" ;

	GDEF_IGNORE_RETURN std::system( ss.str().c_str() ) ;
	return true ;
}

