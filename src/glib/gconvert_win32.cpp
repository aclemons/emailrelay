//
// Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gconvert_win32.cpp
///

#include "gdef.h"
#include "gconvert.h"
#include "gstr.h"
#include <vector>

#if defined(G_MINGW) && !defined(WC_ERR_INVALID_CHARS)
#define WC_ERR_INVALID_CHARS 0
#endif

std::wstring G::Convert::widenImp( const char * p_in , std::size_t n_in )
{
	std::wstring result ;
	if( p_in && n_in )
	{
		DWORD flags = 0 ; // not MB_ERR_INVALID_CHARS
		int n_out_1 = MultiByteToWideChar( CP_UTF8 , flags , p_in , static_cast<int>(n_in) ,
			nullptr , 0 ) ;
		if( n_out_1 <= 0 )
			throw Convert::WidenError() ;

		std::vector<wchar_t> buffer( static_cast<std::size_t>(n_out_1) ) ;
		int n_out_2 = MultiByteToWideChar( CP_UTF8 , flags , p_in , static_cast<int>(n_in) ,
			buffer.data() , static_cast<int>(buffer.size()) ) ;
		if( n_out_2 != n_out_1 )
			throw Convert::WidenError() ;

		result = std::wstring( buffer.data() , buffer.size() ) ;
	}
	return result ;
}

std::string G::Convert::narrowImp( const wchar_t * p_in , std::size_t n_in )
{
	std::string result ;
	if( p_in && n_in )
	{
		DWORD flags = 0 ; // not MB_ERR_INVALID_CHARS
		int n_out_1 = WideCharToMultiByte( CP_UTF8 , flags , p_in , static_cast<int>(n_in) ,
			nullptr , 0 , nullptr , nullptr ) ;
		if( n_out_1 <= 0 )
			throw Convert::NarrowError() ;

		std::vector<char> buffer( static_cast<std::size_t>(n_out_1) ) ;
		int n_out_2 = WideCharToMultiByte( CP_UTF8 , flags , p_in , static_cast<int>(n_in) ,
			buffer.data() , static_cast<int>(buffer.size()) , nullptr , nullptr ) ;
		if( n_out_2 != n_out_1 )
			throw Convert::NarrowError() ;

		result = std::string( buffer.data() , buffer.size() ) ;
	}
	return result ;
}

bool G::Convert::invalid( const std::wstring & s )
{
	return s.find( L'\xFFFD' ) != std::string::npos ;
}

bool G::Convert::invalid( const std::string & s )
{
	return s.find( "\xEF\xBF\xBD" , 0U , 3U ) != std::string::npos ;
}

