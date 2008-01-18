//
// Copyright (C) 2001-2008 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include <string>
#include <sstream>
#include <fstream>

class GLinkImp 
{
public:
	GLinkImp( const G::Path & target_path , const std::string & name , const std::string & description , 
		const G::Path & working_dir , const std::string & args , const G::Path & icon_source , 
		GLink::Show show ) ;
	static std::string filename( const std::string & ) ;
	void saveAs( const G::Path & ) ;

private:
	GLinkImp( const GLinkImp & ) ;
	void operator=( const GLinkImp & ) ;
} ;

GLinkImp::GLinkImp( const G::Path & target_path , const std::string & name , const std::string & description , 
	const G::Path & working_dir , const std::string & args , const G::Path & icon_source , GLink::Show show )
{
}

std::string GLinkImp::filename( const std::string & name )
{
	return std::string() ;
}

void GLinkImp::saveAs( const G::Path & path )
{
}

// ==

GLink::GLink( const G::Path & target_path , const std::string & name , const std::string & description , 
	const G::Path & working_dir , const std::string & args , const G::Path & icon_source , Show show ) :
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

/// \file glink_mac.cpp
