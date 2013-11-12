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
// glink_win32.cpp
//

#include "gdef.h"
#include "gconvert.h"
#include "glink.h"
#include "gstr.h"
#include "gfile.h"
#include <stdexcept>
#include <string>
#include <sstream>
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
		std::wstring ws ;
		G::Convert::convert( ws , s ) ;
		m_p = SysAllocString( ws.c_str() ) ;
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
		const G::Path & working_dir , const G::Strings & args , const G::Path & icon_source , GLink::Show show ) ;
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
	void setArgs( const G::Strings & ) ;
	void setIcon( const G::Path & ) ;
	void setShow( int ) ;

private:
	GComPtr<IShellLink> m_ilink ;
	GComPtr<IPersistFile> m_ipf ;
} ;

GLinkImp::GLinkImp( const G::Path & target_path , const std::string & , const std::string & description ,
	const G::Path & working_dir , const G::Strings & args , const G::Path & icon_source , GLink::Show show_enum )
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
	std::basic_string<TCHAR> arg ;
	G::Convert::convert( arg , target_path.str() ) ;
	HRESULT hr = m_ilink.get()->SetPath( arg.c_str() ) ;
	check( hr , "SetPath" ) ;
}

void GLinkImp::setWorkingDir( const G::Path & working_dir )
{
	std::basic_string<TCHAR> arg ;
	G::Convert::convert( arg , working_dir.str() ) ;
	HRESULT hr = m_ilink.get()->SetWorkingDirectory( arg.c_str() ) ;
	check( hr , "SetWorkingDirectory" ) ;
}

void GLinkImp::setDescription( const std::string & s )
{
	std::basic_string<TCHAR> arg ;
	G::Convert::convert( arg , s ) ;
	HRESULT hr = m_ilink.get()->SetDescription( arg.c_str() ) ;
	check( hr , "SetDescription" ) ;
}

void GLinkImp::setArgs( const G::Strings & args )
{
	std::ostringstream ss ;
	const char * sep = "" ;
	for( G::Strings::const_iterator p = args.begin() ; p != args.end() ; ++p )
	{
		std::string s = *p ;
		const char * qq = "" ;
		if( s.find(' ') != std::string::npos )
		{
			G::Str::replaceAll( s , "\"" , "\\\"" ) ; // windows is too stupid for this to work :-<
			qq = "\"" ;
		}
		ss << sep << qq << s << qq ;
		sep = " " ;
	}

	std::basic_string<TCHAR> arg ;
	G::Convert::convert( arg , ss.str() ) ;
	HRESULT hr = m_ilink.get()->SetArguments( arg.c_str() ) ;
	check( hr , "SetArguments" ) ;
}

void GLinkImp::setIcon( const G::Path & icon_source )
{
	std::basic_string<TCHAR> arg ;
	G::Convert::convert( arg , icon_source.str() ) ;
	HRESULT hr = m_ilink.get()->SetIconLocation( arg.c_str() , 0U ) ;
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

bool GLink::remove( const G::Path & link_path )
{
	return G::File::remove( link_path , G::File::NoThrow() ) ;
}

/// \file glink_win32.cpp
