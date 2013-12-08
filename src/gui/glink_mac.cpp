//
// Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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
//
// glink_mac.cpp
//

#include "gdef.h"
#include "gstr.h"
#include "glink.h"
#include "gfile.h"
#include <string>
#include <sstream>
#include <fstream>
#include <stdlib.h> // system()

class GLinkImp 
{
public:
	GLinkImp( const G::Path & target_path , const std::string & name , const std::string & description ,
		const G::Path & working_dir , const G::Strings & args , const G::Path & icon_source , GLink::Show show ) ;
	static std::string filename( const std::string & ) ;
	void saveAs( const G::Path & ) ;

private:
	GLinkImp( const GLinkImp & ) ;
	void operator=( const GLinkImp & ) ;

private:
	G::Path m_target_path ;
	std::string m_name ;
} ;

GLinkImp::GLinkImp( const G::Path & target_path , const std::string & name , const std::string & ,
	const G::Path & , const G::Strings & , const G::Path & , GLink::Show ) :
		m_target_path(target_path) ,
		m_name(name)
{
}

std::string GLinkImp::filename( const std::string & )
{
	return std::string() ;
}

void GLinkImp::saveAs( const G::Path & )
{
	// TODO -- hardcoded Start.app name
	//
	// we have both a start/stop script and a startup application bundle 
	// and here we need to convert one to the other -- should maybe expose 
	// the installer's LinkInfo structure at this interface and add a 
	// third section to it
	//
	G::Path start_app_path( m_target_path.dirname() , "E-MailRelay-Start.app" ) ;
	if( !G::File::exists(start_app_path) )
		start_app_path = G::Path( m_target_path.dirname() , "../E-MailRelay-Start.app" ) ;

	std::ostringstream ss ;
	ss
		<< "/usr/bin/osascript "
			<< "-e \"tell application \\\"System Events\\\"\" "
			<< "-e \"make new login item at end of login items with properties {"
				<< "path:\\\"" << start_app_path << "\\\","
				<< "hidden:true}\" "
			<< "-e \"end tell\"" ;

	int rc = system( ss.str().c_str() ) ;
	G_IGNORE_VARIABLE( rc ) ;
}

// ==

GLink::GLink( const G::Path & target_path , const std::string & name , const std::string & description ,
	const G::Path & working_dir , const G::Strings & args , const G::Path & icon_source , Show show ,
	const std::string & , const std::string & , const std::string & ) :
		m_imp( new GLinkImp(target_path,name,description,working_dir,args,icon_source,show) )
{
}

std::string GLink::filename( const std::string & name_in )
{
	return GLinkImp::filename( name_in ) ;
}

void GLink::saveAs( const G::Path & link_path )
{
	m_imp->saveAs( link_path ) ;
}

GLink::~GLink()
{
	delete m_imp ;
}

bool GLink::remove( const G::Path & )
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

	int rc = system( ss.str().c_str() ) ;
	G_IGNORE_VARIABLE( rc ) ;
	return true ;
}

/// \file glink_mac.cpp
