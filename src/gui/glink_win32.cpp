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
//
// glink_win32.cpp
//

#include "gdef.h"
#include "gconvert.h"
#include "glink.h"
#include "gstr.h"
#include "gfile.h"
#include "gcominit.h"
#include <stdexcept>
#include <string>
#include <sstream>
#include <shlwapi.h>
#include <shlobj.h>
#include <windows.h>

namespace G
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
		void operator=( const ComPtr<I> & ) = delete ;
		void operator=( ComPtr<I> && ) = delete ;
		private: I * m_p ;
	} ;
}

class G::LinkImp
{
public:
	LinkImp( const Path & target_path , const std::string & name , const std::string & description ,
		const Path & working_dir , const StringArray & args , const Path & icon_source , Link::Show show ) ;
	static std::string filename( const std::string & ) ;
	void saveAs( const Path & link_path ) ;

public:
	~LinkImp() = default ;
	LinkImp( const LinkImp & ) = delete ;
	LinkImp( LinkImp && ) = delete ;
	void operator=( const LinkImp & ) = delete ;
	void operator=( LinkImp && ) = delete ;

private:
	static void check( HRESULT , const char * ) ;
	void createInstance() ;
	void qi() ;
	void setTargetPath( const Path & ) ;
	void setDescription( const std::string & s ) ;
	void setWorkingDir( const Path & ) ;
	void setArgs( const StringArray & ) ;
	void setIcon( const Path & ) ;
	void setShow( int ) ;

private:
	GComInit m_com_init ;
	ComPtr<IShellLink> m_ilink ;
	ComPtr<IPersistFile> m_ipf ;

private:
	struct bstr
	{
		explicit bstr( const std::string & s ) ;
		~bstr()  ;
		BSTR p() ;
		bstr( const bstr & ) = delete ;
		bstr( bstr && ) = delete ;
		void operator=( const bstr & ) = delete ;
		void operator=( bstr && ) = delete ;
		private: BSTR m_p ;
	} ;
} ;

G::LinkImp::bstr::bstr( const std::string & s )
{
	std::wstring ws ;
	Convert::convert( ws , s ) ;
	m_p = SysAllocString( ws.c_str() ) ;
}

G::LinkImp::bstr::~bstr()
{
	SysFreeString( m_p ) ;
}

BSTR G::LinkImp::bstr::p()
{
	return m_p ;
}

// ==

G::LinkImp::LinkImp( const Path & target_path , const std::string & , const std::string & description ,
	const Path & working_dir , const StringArray & args , const Path & icon_source , Link::Show show_enum )
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

std::string G::LinkImp::filename( const std::string & name_in )
{
	return name_in + ".lnk" ;
}

void G::LinkImp::check( HRESULT hr , const char * op )
{
	if( FAILED(hr) )
	{
		std::ostringstream ss ;
		ss << "com error: " << op << ": " << std::hex << hr ;
		if( hr == E_ACCESSDENIED ) ss << " (access denied)" ;
		throw Link::SaveError( ss.str() ) ;
	}
}

void G::LinkImp::createInstance()
{
	HRESULT hr = CoCreateInstance( CLSID_ShellLink , nullptr , CLSCTX_INPROC_SERVER , IID_IShellLink , m_ilink.vp() ) ;
	check( hr , "createInstance" ) ;
}

void G::LinkImp::qi()
{
	HRESULT hr = m_ilink.get()->QueryInterface( IID_IPersistFile , m_ipf.vp() ) ;
	check( hr , "qi" ) ;
}

void G::LinkImp::setTargetPath( const Path & target_path )
{
	std::basic_string<TCHAR> arg ;
	Convert::convert( arg , target_path.str() ) ;
	HRESULT hr = m_ilink.get()->SetPath( arg.c_str() ) ;
	check( hr , "SetPath" ) ;
}

void G::LinkImp::setWorkingDir( const Path & working_dir )
{
	std::basic_string<TCHAR> arg ;
	Convert::convert( arg , working_dir.str() ) ;
	HRESULT hr = m_ilink.get()->SetWorkingDirectory( arg.c_str() ) ;
	check( hr , "SetWorkingDirectory" ) ;
}

void G::LinkImp::setDescription( const std::string & s )
{
	std::basic_string<TCHAR> arg ;
	Convert::convert( arg , s ) ;
	HRESULT hr = m_ilink.get()->SetDescription( arg.c_str() ) ;
	check( hr , "SetDescription" ) ;
}

void G::LinkImp::setArgs( const StringArray & args )
{
	std::ostringstream ss ;
	const char * sep = "" ;
	for( StringArray::const_iterator p = args.begin() ; p != args.end() ; ++p )
	{
		std::string s = *p ;
		const char * qq = "" ;
		if( s.find(' ') != std::string::npos )
		{
			Str::replaceAll( s , "\"" , "\\\"" ) ; // windows is too stupid for this to work :-<
			qq = "\"" ;
		}
		ss << sep << qq << s << qq ;
		sep = " " ;
	}

	std::basic_string<TCHAR> arg ;
	Convert::convert( arg , ss.str() ) ;
	HRESULT hr = m_ilink.get()->SetArguments( arg.c_str() ) ;
	check( hr , "SetArguments" ) ;
}

void G::LinkImp::setIcon( const Path & icon_source )
{
	std::basic_string<TCHAR> arg ;
	Convert::convert( arg , icon_source.str() ) ;
	HRESULT hr = m_ilink.get()->SetIconLocation( arg.c_str() , 0U ) ;
	check( hr , "SetIconLocation" ) ;
}

void G::LinkImp::setShow( int show )
{
	HRESULT hr = m_ilink.get()->SetShowCmd( show ) ;
	check( hr , "SetShowCmd" ) ;
}

void G::LinkImp::saveAs( const Path & link_path )
{
	HRESULT hr = m_ipf.get()->Save( bstr(link_path.str()).p() , TRUE ) ;
	check( hr , "Save" ) ;
}

// ==

G::Link::Link( const Path & target_path , const std::string & name , const std::string & description ,
	const Path & working_dir , const StringArray & args , const Path & icon_source , Show show ,
	const std::string & , const std::string & , const std::string & ) :
		m_imp(std::make_unique<LinkImp>(target_path,name,description,working_dir,args,icon_source,show))
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
	return File::exists( path , std::nothrow ) ;
}

bool G::Link::remove( const Path & link_path )
{
	return File::remove( link_path , std::nothrow ) ;
}

/// \file glink_win32.cpp
