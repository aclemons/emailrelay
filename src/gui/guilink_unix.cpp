//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file guilink_unix.cpp
///

#include "gdef.h"
#include "guilink.h"
#include "gstr.h"
#include "gfile.h"
#include <string>
#include <sstream>
#include <fstream>

class Gui::LinkImp
{
public:
	LinkImp( const G::Path & target_path , const std::string & name , const std::string & description ,
		const G::Path & working_dir , const G::StringArray & args , const G::Path & icon_source , Link::Show show ,
		const std::string & c1 , const std::string & c2 , const std::string & c3 ) ;
	static std::string filename( const std::string & ) ;
	void saveAs( const G::Path & ) ;

public:
	~LinkImp() = default ;
	LinkImp( const LinkImp & ) = delete ;
	LinkImp( LinkImp && ) = delete ;
	LinkImp & operator=( const LinkImp & ) = delete ;
	LinkImp & operator=( LinkImp && ) = delete ;

private:
	static std::string quote( const std::string & ) ;
	static std::string escape( const std::string & ) ;
	static std::string escapeAndQuote( const G::StringArray & ) ;

private:
	G::Path m_target_path ;
	std::string m_name ;
	std::string m_description ;
	G::Path m_working_dir ;
	G::StringArray m_args ;
	G::Path m_icon_source ;
	bool m_terminal ;
	std::string m_c1 ;
	std::string m_c2 ;
	std::string m_c3 ;
} ;

Gui::LinkImp::LinkImp( const G::Path & target_path , const std::string & name , const std::string & description ,
	const G::Path & working_dir , const G::StringArray & args , const G::Path & icon_source , Link::Show show ,
	const std::string & c1 , const std::string & c2 , const std::string & c3 ) :
		m_target_path(target_path) ,
		m_name(name) ,
		m_description(description) ,
		m_working_dir(working_dir) ,
		m_args(args) ,
		m_icon_source(icon_source) ,
		m_terminal(show==Link::Show::Default) ,
		m_c1(c1) ,
		m_c2(c2) ,
		m_c3(c3)
{
}

std::string Gui::LinkImp::filename( const std::string & name )
{
	std::string result = G::Str::lower(name) + ".desktop" ;
	G::Str::replaceAll( result , "-" , "" ) ;
	return result ;
}

void Gui::LinkImp::saveAs( const G::Path & path )
{
	// see "http://standards.freedesktop.org"

	// TODO maybe use "xdg-desktop-menu"/"xdg-desktop-icon"

	const char * eol = "\n" ;
	std::ofstream file ;
	G::File::open( file , path ) ;

	if( !m_c1.empty() ) file << "# " << m_c1 << eol ;
	if( !m_c2.empty() ) file << "# " << m_c2 << eol ;
	if( !m_c3.empty() ) file << "# " << m_c3 << eol ;

	file << "[Desktop Entry]" << eol ;
	file << "Type=Application" << eol ;
	file << "Version=1.0" << eol ;
	//file << "Encoding=UTF-8" << eol ;
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
		throw Link::SaveError(path.str()) ;

	G::File::chmodx( path , std::nothrow ) ;
}

std::string Gui::LinkImp::escape( const std::string & s_in )
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

	return G::Str::escaped( G::Str::escaped(s_in,'\\',"\\$","\\$") , '%' , "%" , "%" ) ;
}

std::string Gui::LinkImp::quote( const std::string & s_in )
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

		return std::string(1U,'\"') + G::Str::escaped(s_in,'\\',"\"`$\\","\"`$\\") + std::string(1U,'\"') ;
	}
	else
	{
		return s_in ;
	}
}

std::string Gui::LinkImp::escapeAndQuote( const G::StringArray & args )
{
	std::ostringstream ss ;
	const char * sep = "" ;
	for( const auto & arg : args )
	{
		ss << sep << quote(escape(arg)) ;
		sep = " " ;
	}
	return ss.str() ;
}

// ==

Gui::Link::Link( const G::Path & target_path , const std::string & name , const std::string & description ,
	const G::Path & working_dir , const G::StringArray & args , const G::Path & icon_source , Show show ,
	const std::string & c1 , const std::string & c2 , const std::string & c3 ) :
		m_imp(std::make_unique<LinkImp>(target_path,name,description,working_dir,args,icon_source,show,c1,c2,c3))
{
}

Gui::Link::~Link()
= default ;

std::string Gui::Link::filename( const std::string & name_in )
{
	return LinkImp::filename( name_in ) ;
}

void Gui::Link::saveAs( const G::Path & link_path )
{
	m_imp->saveAs( link_path ) ;
}

bool Gui::Link::exists( const G::Path & path )
{
	return G::File::isLink( path , std::nothrow ) ;
}

bool Gui::Link::remove( const G::Path & link_path )
{
	return G::File::remove( link_path , std::nothrow ) ;
}

