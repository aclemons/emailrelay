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
//
// guilink_win32.cpp
//

#include "gdef.h"
#include "gnowide.h"
#include "guilink.h"
#include "gconvert.h"
#include "gstr.h"
#include "gfile.h"
#include "gcominit.h"
#include <stdexcept>
#include <string>
#include <sstream>
#include <shlwapi.h>
#include <shlobj.h>
#include <windows.h>

namespace Gui
{
	template <typename I>
	struct ComPtr
	{
		ComPtr() : m_p(nullptr) {}
		explicit ComPtr( I * p ) : m_p(p) {}
		~ComPtr() { if(m_p) m_p->Release() ; }
		I * get() { return m_p ; }
		const I * get() const { return m_p ; }
		void ** vp() { return reinterpret_cast<void**>(&m_p) ; }
		ComPtr( const ComPtr<I> & ) = delete ;
		ComPtr( ComPtr<I> && ) = delete ;
		ComPtr<I> & operator=( const ComPtr<I> & ) = delete ;
		ComPtr<I> & operator=( ComPtr<I> && ) = delete ;
		private: I * m_p ;
	} ;
}

class Gui::LinkImp
{
public:
	LinkImp( const G::Path & target_path , const std::string & name , const std::string & description ,
		const G::Path & working_dir , const G::StringArray & args , const G::Path & icon_source , Link::Show show ) ;
	static std::string filename( const std::string & ) ;
	void saveAs( const G::Path & link_path ) ;

public:
	~LinkImp() = default ;
	LinkImp( const LinkImp & ) = delete ;
	LinkImp( LinkImp && ) = delete ;
	LinkImp & operator=( const LinkImp & ) = delete ;
	LinkImp & operator=( LinkImp && ) = delete ;

private:
	static void check( HRESULT , const char * ) ;
	void createInstance() ;
	void qi() ;
	void setTargetPath( const G::Path & ) ;
	void setDescription( const std::string & s ) ;
	void setWorkingDir( const G::Path & ) ;
	void setArgs( const G::StringArray & ) ;
	void setIcon( const G::Path & ) ;
	void setShow( int ) ;

private:
	GComInit m_com_init ;
	ComPtr<G::nowide::IShellLink_type> m_ilink ;
	ComPtr<IPersistFile> m_ipf ;
} ;

// ==

Gui::LinkImp::LinkImp( const G::Path & target_path , const std::string & , const std::string & description ,
	const G::Path & working_dir , const G::StringArray & args , const G::Path & icon_source , Link::Show show_enum )
{
	createInstance() ;
	setTargetPath( target_path ) ;
	if( !description.empty() ) setDescription( description ) ;
	if( !working_dir.str().empty() ) setWorkingDir( working_dir ) ;
	if( !args.empty() ) setArgs( args ) ;
	if( !icon_source.empty() ) setIcon( icon_source ) ;
	if( show_enum == Link::Show::Hide ) setShow( SW_HIDE ) ;
	qi() ;
}

std::string Gui::LinkImp::filename( const std::string & name_in )
{
	return name_in + ".lnk" ;
}

void Gui::LinkImp::check( HRESULT hr , const char * op )
{
	if( FAILED(hr) )
	{
		std::ostringstream ss ;
		ss << "com error: " << op << ": " << std::hex << hr ;
		if( hr == E_ACCESSDENIED ) ss << " (access denied)" ;
		throw Link::SaveError( ss.str() ) ;
	}
}

void Gui::LinkImp::createInstance()
{
	HRESULT hr = CoCreateInstance( CLSID_ShellLink , nullptr , CLSCTX_INPROC_SERVER ,
		G::nowide::iidShellLink() , m_ilink.vp() ) ;
	check( hr , "createInstance" ) ;
}

void Gui::LinkImp::qi()
{
	HRESULT hr = m_ilink.get()->QueryInterface( IID_IPersistFile , m_ipf.vp() ) ;
	check( hr , "qi" ) ;
}

void Gui::LinkImp::setTargetPath( const G::Path & target_path )
{
	HRESULT hr = G::nowide::shellLinkSetPath( m_ilink.get() , target_path ) ;
	check( hr , "SetPath" ) ;
}

void Gui::LinkImp::setWorkingDir( const G::Path & working_dir )
{
	HRESULT hr = G::nowide::shellLinkSetWorkingDirectory( m_ilink.get() , working_dir ) ;
	check( hr , "SetWorkingDirectory" ) ;
}

void Gui::LinkImp::setDescription( const std::string & s )
{
	HRESULT hr = G::nowide::shellLinkSetDescription( m_ilink.get() , s ) ;
	check( hr , "SetDescription" ) ;
}

void Gui::LinkImp::setArgs( const G::StringArray & args )
{
	std::ostringstream ss ;
	const char * sep = "" ;
	for( G::StringArray::const_iterator p = args.begin() ; p != args.end() ; ++p )
	{
		std::string s = *p ;
		const char * qq = "" ;
		if( s.find(' ') != std::string::npos )
			qq = "\"" ;
		ss << sep << qq << s << qq ;
		sep = " " ;
	}

	HRESULT hr = G::nowide::shellLinkSetArguments( m_ilink.get() , ss.str() ) ;
	check( hr , "SetArguments" ) ;
}

void Gui::LinkImp::setIcon( const G::Path & icon_source )
{
	HRESULT hr = G::nowide::shellLinkSetIconLocation( m_ilink.get() , icon_source , 0U ) ;
	check( hr , "SetIconLocation" ) ;
}

void Gui::LinkImp::setShow( int show )
{
	HRESULT hr = G::nowide::shellLinkSetShowCmd( m_ilink.get() , show ) ;
	check( hr , "SetShowCmd" ) ;
}

void Gui::LinkImp::saveAs( const G::Path & link_path )
{
	HRESULT hr = G::nowide::persistFileSave( m_ipf.get() , link_path , TRUE ) ;
	check( hr , "Save" ) ;
}

// ==

Gui::Link::Link( const G::Path & target_path , const std::string & name , const std::string & description ,
	const G::Path & working_dir , const G::StringArray & args , const G::Path & icon_source , Show show ,
	const std::string & , const std::string & , const std::string & ) :
		m_imp(std::make_unique<LinkImp>(target_path,name,description,working_dir,args,icon_source,show))
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
	return G::File::exists( path , std::nothrow ) ;
}

bool Gui::Link::remove( const G::Path & link_path )
{
	return G::File::remove( link_path , std::nothrow ) ;
}

/// \file glink_win32.cpp
