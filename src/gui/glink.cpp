//
// Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// glink.cpp
//

#include "gdef.h"
#include "gstr.h"
#include "glink.h"
#include <stdexcept>
#include <string>
#include <sstream>
#include <iostream>

#ifdef _WIN32

#define _WIN32_IE 0x600
#include <shlwapi.h>
#include <shlobj.h>
#include <windows.h>

template <typename I>
struct GComPtr
{
	I * m_p ;
	GComPtr() : m_p(NULL) {}
	explicit GComPtr( I * p ) : m_p(p) {}
	~GComPtr() { if(m_p) m_p->Release() ; }
	I * get() { return m_p ; }
	const I * get() const { return m_p ; }
	void ** vp() { return (void**) &m_p ; }
	private: GComPtr(const GComPtr<I> &) ;
	private: void operator=(const GComPtr<I> &) ;
} ;

struct bstr
{
	private: BSTR m_p ;
	public: explicit bstr( const std::string & s ) 
	{ 
		int n = MultiByteToWideChar( CP_ACP , 0 , s.c_str() , -1 , NULL , 0 ) ;
		if( n == 0 ) throw std::runtime_error("string conversion error") ;
		OLECHAR * p = new OLECHAR[n*2+10] ;
		n = MultiByteToWideChar( CP_ACP , 0 , s.c_str() , -1 , p , n ) ;
		if( n == 0 ) { delete [] p ; throw std::runtime_error("string conversion error") ; }
		m_p = SysAllocString( p ) ;
		delete [] p ;
	}
	public: ~bstr() { SysFreeString(m_p) ; }
	public: BSTR p() { return m_p ; }
	private: bstr( const bstr & ) ;
	private: void operator=( const bstr & ) ;
	
} ;

class GLinkImp 
{
public:
	GLinkImp( const G::Path & target_path , const std::string & name , const std::string & description , 
		const G::Path & working_dir , const std::string & args , const G::Path & icon_source , 
		GLink::Show show ) ;
	static std::string filename( const std::string & ) ;
	void saveAs( const G::Path & link_path ) ;

private:
	GLinkImp( const GLinkImp & ) ;
	void operator=( const GLinkImp & ) ;
	static void check( HRESULT , const char * ) ;
	void createInstance() ;
	void qi() ;
	void setTargetPath( const G::Path & ) ;
	void setDescription( const std::string & s ) ;
	void setWorkingDir( const G::Path & ) ;
	void setArgs( const std::string & ) ;
	void setIcon( const G::Path & ) ;
	void setShow( int ) ;

private:
	GComPtr<IShellLink> m_ilink ;
	GComPtr<IPersistFile> m_ipf ;
} ;

GLinkImp::GLinkImp( const G::Path & target_path , const std::string & , const std::string & description , 
	const G::Path & working_dir , const std::string & args , const G::Path & icon_source , GLink::Show show_enum )
{
	createInstance() ;
	setTargetPath( target_path ) ;
	if( ! description.empty() ) setDescription( description ) ;
	if( ! working_dir.str().empty() ) setWorkingDir( working_dir ) ;
	if( ! args.empty() ) setArgs( args ) ;
	if( icon_source != G::Path() ) setIcon( icon_source ) ;
	if( show_enum == GLink::Show_Hide ) setShow( SW_HIDE ) ;
	qi() ;
}

std::string GLinkImp::filename( const std::string & name_in )
{
	return name_in + ".lnk" ;
}

void GLinkImp::check( HRESULT hr , const char * op )
{
	if( FAILED(hr) )
	{
		std::ostringstream ss ;
		ss << "com error: " << op << ": " << std::hex << hr ;
		if( hr == E_ACCESSDENIED ) ss << " (access denied)" ;
		throw GLink::SaveError( ss.str() ) ;
	}
}

void GLinkImp::createInstance()
{
	HRESULT hr = CoCreateInstance( CLSID_ShellLink , NULL , CLSCTX_INPROC_SERVER , IID_IShellLink , m_ilink.vp() ) ;
	check( hr , "createInstance" ) ;
}

void GLinkImp::qi()
{
	HRESULT hr = m_ilink.get()->QueryInterface( IID_IPersistFile , m_ipf.vp() ) ;
	check( hr , "qi" ) ;
}

void GLinkImp::setTargetPath( const G::Path & target_path )
{
	HRESULT hr = m_ilink.get()->SetPath( target_path.str().c_str() ) ;
	check( hr , "SetPath" ) ;
}

void GLinkImp::setWorkingDir( const G::Path & working_dir )
{
	HRESULT hr = m_ilink.get()->SetWorkingDirectory( working_dir.str().c_str() ) ;
	check( hr , "SetWorkingDirectory" ) ;
}

void GLinkImp::setDescription( const std::string & s )
{
	HRESULT hr = m_ilink.get()->SetDescription( s.c_str() ) ;
	check( hr , "SetDescription" ) ;
}

void GLinkImp::setArgs( const std::string & s )
{
	HRESULT hr = m_ilink.get()->SetArguments( s.c_str() ) ;
	check( hr , "SetArguments" ) ;
}

void GLinkImp::setIcon( const G::Path & icon_source )
{
	HRESULT hr = m_ilink.get()->SetIconLocation( icon_source.str().c_str() , 0U ) ;
	check( hr , "SetIconLocation" ) ;
}

void GLinkImp::setShow( int show )
{
	HRESULT hr = m_ilink.get()->SetShowCmd( show ) ;
	check( hr , "SetShowCmd" ) ;
}

void GLinkImp::saveAs( const G::Path & link_path )
{
	HRESULT hr = m_ipf.get()->Save( bstr(link_path.str()).p() , TRUE ) ;
	check( hr , "Save" ) ;
}

#else

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

private:
	G::Path m_target_path ;
	std::string m_name ;
	std::string m_description ;
	G::Path m_working_dir ;
	std::string m_args ;
	G::Path m_icon_source ;
	bool m_terminal ;
} ;

GLinkImp::GLinkImp( const G::Path & target_path , const std::string & name , const std::string & description , 
	const G::Path & working_dir , const std::string & args , const G::Path & icon_source , GLink::Show show ) :
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

	file << "Exec=" << m_target_path << " " << m_args << eol ; // TODO -- proper escaping of %
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

#endif

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

/// \file glink.cpp
