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
// gconvert_win32.cpp
//

#include "gdef.h"
#include "gconvert.h"
#include "gstr.h"

static std::string message( const std::string & context , DWORD e , const std::string & ascii )
{
	std::ostringstream ss ;
	ss << context << (context.empty()?"":": ") << e << ": [" << ascii << "]" ;
	return ss.str() ;
}

std::wstring G::Convert::widen( const std::string & s , bool is_utf8 , const std::string & context )
{
	unsigned int codepage = is_utf8 ? CP_UTF8 : CP_ACP ;
	std::wstring result ;
	if( ! s.empty() ) 
	{
		DWORD flags = MB_ERR_INVALID_CHARS ;
		int n = MultiByteToWideChar( codepage , flags , s.c_str() , static_cast<int>(s.size()) , NULL , 0 ) ;
		if( n == 0 ) 
		{
			DWORD e = ::GetLastError() ;
			throw G::Convert::Error( message(context,e,G::Str::toPrintableAscii(s)) ) ;
		}

		wchar_t * buffer = new wchar_t[n] ;
		n = MultiByteToWideChar( codepage , flags , s.c_str() , static_cast<int>(s.size()) , buffer , n ) ;
		if( n == 0 ) 
		{ 
			DWORD e = ::GetLastError() ;
			delete [] buffer ; 
			throw G::Convert::Error( message(context,e,G::Str::toPrintableAscii(s)) ) ;
		}
		result = std::wstring( buffer , n ) ;
		delete [] buffer ;
	}
	return result ;
}

std::string G::Convert::narrow( const std::wstring & s , bool is_utf8 , const std::string & context )
{
	unsigned int codepage = is_utf8 ? CP_UTF8 : CP_ACP ;
	std::string result ;
	if( ! s.empty() )
	{
#if defined(G_MINGW) || defined(G_COMPILER_IS_OLD)
		DWORD flags = 0 ;
#else
		DWORD flags = is_utf8 ? WC_ERR_INVALID_CHARS : 0 ;
#endif
		BOOL defaulted = FALSE ;
		int n = WideCharToMultiByte( codepage , flags , s.c_str() , static_cast<int>(s.size()) , NULL , 0 , 
			NULL , is_utf8 ? NULL : &defaulted ) ;
		if( n == 0 || defaulted )
		{
			DWORD e = n == 0 ? ::GetLastError() : 0 ;
			throw G::Convert::Error( message(context,e,G::Str::toPrintableAscii(s)) ) ;
		}

		char * buffer = new char[n] ;
		n = WideCharToMultiByte( codepage , flags , s.c_str() , static_cast<int>(s.size()) , buffer , n , 
			NULL , is_utf8 ? NULL : &defaulted ) ;
		if( n == 0 || defaulted )
		{ 
			DWORD e = n == 0 ? ::GetLastError() : 0 ;
			delete [] buffer ; 
			throw G::Convert::Error( message(context,e,G::Str::toPrintableAscii(s)) ) ;
		}
		result = std::string( buffer , n ) ;
		delete [] buffer ;
	}
	return result ;
}

/// \file gconvert_win32.cpp
