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
// glink_unix.cpp
//

#include "gdef.h"
#include "gstr.h"
#include "glink.h"
#include "gfile.h"
#include <string>
#include <sstream>
#include <fstream>

class GLinkImp 
{
public:
	GLinkImp( const G::Path & target_path , const std::string & name , const std::string & description , 
		const G::Path & working_dir , const G::Strings & args , const G::Path & icon_source , 
		GLink::Show show ) ;
	static std::string filename( const std::string & ) ;
	void saveAs( const G::Path & ) ;

private:
	GLinkImp( const GLinkImp & ) ;
	void operator=( const GLinkImp & ) ;
	static std::string quote( const std::string & ) ;
	static std::string escape( const std::string & ) ;
	static std::string escapeAndQuote( const G::Strings & ) ;

private:
	G::Path m_target_path ;
	std::string m_name ;
	std::string m_description ;
	G::Path m_working_dir ;
	G::Strings m_args ;
	G::Path m_icon_source ;
	bool m_terminal ;
} ;

GLinkImp::GLinkImp( const G::Path & target_path , const std::string & name , const std::string & description , 
	const G::Path & working_dir , const G::Strings & args , const G::Path & icon_source , GLink::Show show ) :
		m_target_path(target_path) ,
		m_name(name) ,
		m_description(description) ,
		m_working_dir(working_dir) ,
		m_args(args) ,
		m_icon_source(icon_source) ,
		m_terminal(show==GLink::Show_Default)
{
}

std::string GLinkImp::filename( const std::string & name )
{
	std::string result = G::Str::lower(name) + ".desktop" ;
	G::Str::replaceAll( result , "-" , "" ) ;
	return result ;
}

void GLinkImp::saveAs( const G::Path & path )
{
	// see "http://standards.freedesktop.org"

	const char * eol = "\n" ;
	std::ofstream file( path.str().c_str() ) ;
	file << "[Desktop Entry]" << eol ;
	file << "Type=Application" << eol ;
	file << "Version=1.0" << eol ;
	file << "Encoding=UTF-8" << eol ;
	file << "StartupNotify=false" << eol ;

	file << "Exec=" << quote(escape(m_target_path.str())) << " " << escapeAndQuote(m_args) << eol ;
	file << "Name=" << m_name << eol ;
	file << "Comment=" << m_description << eol ;
	file << "Path=" << m_working_dir << eol ;
	if( ! m_icon_source.str().empty() )
	file << "Icon=" << m_icon_source << eol ;
	file << "Terminal=" << (m_terminal?"true":"false") << eol ;

	file << "Categories=System;" << eol ;

	file.flush() ;
	if( !file.good() )
		throw GLink::SaveError(path.str()) ;
}

std::string GLinkImp::escape( const std::string & s_in )
{
	// <citation version="1.0">
	// Note that the general escape rule for values of type string states that 
	// the backslash character can be escaped as ("\\") as well and that this 
	// escape rule is applied before the quoting rule. As such, to unambiguously 
	// represent a literal backslash character in a quoted argument in a desktop 
	// entry file requires the use of four successive backslash characters ("\\\\"). 
	// Likewise, a literal dollar sign in a quoted argument in a desktop entry file 
	// is unambiguously represented with ("\\$").
	// [...]
	// A number of special field codes have been defined [...]. Field codes consist 
	// of the percentage character ("%") followed by an alpha character. Literal 
	// percentage characters must be escaped as %%.
	// </citation>

	return G::Str::escaped( G::Str::escaped(s_in,"\\$") , "%" , '%' ) ;
}

std::string GLinkImp::quote( const std::string & s_in )
{
	// <citation version="1.0">
	// If an argument contains a reserved character the argument must be quoted. 
	// [...]
	// Reserved characters are space (" "), tab, newline, double quote, single quote ("'"), 
	// backslash character ("\"), greater-than sign (">"), less-than sign ("<"), tilde ("~"), 
	// vertical bar ("|"), ampersand ("&"), semicolon (";"), dollar sign ("$"), asterisk ("*"), 
	// question mark ("?"), hash mark ("#"), parenthesis ("(") and (")") and 
	// backtick character ("`").
	// </citation>

	if( s_in.find_first_of(" \t\n\"'\\><~|&;$*?#()`") != std::string::npos )
	{
		// <citation version="1.0">
		// Quoting must be done by enclosing the argument between double quotes and escaping 
		// the double quote character, backtick character ("`"), dollar sign ("$") and backslash 
		// character ("\") by preceding it with an additional backslash character. 
		// </citation>

		return std::string(1U,'\"') + G::Str::escaped(s_in,"\"`$\\") + std::string(1U,'\"') ;
	}
	else
	{
		return s_in ;
	}
}

std::string GLinkImp::escapeAndQuote( const G::Strings & args )
{
	std::ostringstream ss ;
	const char * sep = "" ;
	for( G::Strings::const_iterator p = args.begin() ; p != args.end() ; ++p )
	{
		ss << sep << quote(escape(*p)) ;
		sep = " " ;
	}
	return ss.str() ;
}

// ==

GLink::GLink( const G::Path & target_path , const std::string & name , const std::string & description , 
	const G::Path & working_dir , const G::Strings & args , const G::Path & icon_source , Show show ) :
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

bool GLink::remove( const G::Path & link_path )
{
	return G::File::remove( link_path , G::File::NoThrow() ) ;
}

/// \file glink_unix.cpp
