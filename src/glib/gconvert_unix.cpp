//
// Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gconvert_unix.cpp
//

#include "gdef.h"
#include "gconvert.h"
#include "gstr.h"
#include "glog.h"
#include <vector>
#include <iconv.h>

/// \class G::ConvertImp
/// A pimple-pattern implementation class for G::Convert.
///
class G::ConvertImp
{
public:
	ConvertImp( const std::string & to_code , const std::string & from_code ) ;
	~ConvertImp() ;
	std::string narrow( const std::wstring & s , const std::string & context ) ;
	std::wstring widen( const std::string & s , const std::string & context ) ;
	static ConvertImp & utf8_to_utf16() ;
	static ConvertImp & ansi_to_utf16() ;
	static ConvertImp & utf16_to_utf8() ;
	static ConvertImp & utf16_to_ansi() ;

private:
	ConvertImp( const ConvertImp & ) g__eq_delete ;
	void operator=( const ConvertImp & ) g__eq_delete ;
	void reset() ;
	static std::wstring to_wstring( const std::vector<char> & ) ;
	static void from_wstring( std::vector<char> & , const std::wstring & ) ;
	static size_t call( size_t (*fn)(iconv_t,const char**,size_t*,char**,size_t*) ,
		iconv_t m , const char ** inbuf , size_t * inbytesleft ,
		char ** outbuf , size_t * outbytesleft ) ;
	static size_t call( size_t (*fn)(iconv_t,char**,size_t*,char**,size_t*) ,
		iconv_t m , const char ** inbuf , size_t * inbytesleft ,
		char ** outbuf , size_t * outbytesleft ) ;

private:
	iconv_t m ;
} ;

G::ConvertImp::ConvertImp( const std::string & to_code , const std::string & from_code )
{
	m = ::iconv_open( to_code.c_str() , from_code.c_str() ) ;
	if( m == reinterpret_cast<iconv_t>(-1) )
		throw Convert::Error( "iconv_open failed for " + from_code + " -> " + to_code ) ;
}

G::ConvertImp::~ConvertImp()
{
	::iconv_close( m ) ;
}

void G::ConvertImp::reset()
{
	::iconv( m , nullptr , nullptr , nullptr , nullptr ) ;
}

size_t G::ConvertImp::call( size_t (*fn)(iconv_t,const char**,size_t*,char**,size_t*) ,
	iconv_t m , const char ** inbuf , size_t * inbytesleft ,
	char ** outbuf , size_t * outbytesleft )
{
	return (*fn)( m , inbuf , inbytesleft , outbuf , outbytesleft ) ;
}

size_t G::ConvertImp::call( size_t (*fn)(iconv_t,char**,size_t*,char**,size_t*) ,
	iconv_t m , const char ** inbuf , size_t * inbytesleft ,
	char ** outbuf , size_t * outbytesleft )
{
	char * p = const_cast<char*>(*inbuf) ;
	size_t rc = (*fn)( m , &p , inbytesleft , outbuf , outbytesleft ) ;
	*inbuf = p ;
	return rc ;
}

std::wstring G::ConvertImp::widen( const std::string & s , const std::string & context )
{
	reset() ;

	// in-buffer
	const char * in_p_end = s.data() + s.length() ;
	const char * in_p_start = s.data() ;
	size_t in_n_start = s.size() ;
	const char * in_p = in_p_start ;
	size_t in_n = in_n_start ;

	// out-buffer
	std::vector<char> out_buffer( 10U + s.size()*4U ) ; //kiss
	char * out_p_start = &out_buffer[0] ;
	size_t out_n_start = out_buffer.size() ;
	char * out_p = out_p_start ;
	size_t out_n = out_n_start ;

	// iconv()
	//G_DEBUG( "G::ConvertImp::widen: in...\n" << hexdump<16>(in_p_start,in_p_end) ) ;
	size_t rc = call( ::iconv , m , &in_p , &in_n , &out_p , &out_n ) ;
	const size_t e = static_cast<size_t>(ssize_t(-1)) ;
	if( rc == e || in_p != in_p_end || out_n > out_n_start )
		throw Convert::Error( "iconv failed" + std::string(context.empty()?"":": ") + context ) ;
	//G_DEBUG( "G::ConvertImp::widen: out...\n" << hexdump<16>(out_p_start,out_p) ) ;

	out_buffer.resize( out_n_start - out_n ) ;
	return to_wstring( out_buffer ) ;
}

std::string G::ConvertImp::narrow( const std::wstring & s , const std::string & context )
{
	reset() ;

	// in-buffer
	std::vector<char> in_buffer ;
	from_wstring( in_buffer , s ) ;
	const char * in_p_end = &in_buffer[0] + in_buffer.size() ;
	const char * in_p_start = &in_buffer[0] ;
	size_t in_n_start = in_buffer.size() ;
	const char * in_p = in_p_start ;
	size_t in_n = in_n_start ;

	// out-buffer
	std::vector<char> out_buffer( 10U + in_buffer.size()*4U ) ; //kiss
	char * out_p_start = &out_buffer[0] ;
	size_t out_n_start = out_buffer.size() ;
	char * out_p = out_p_start ;
	size_t out_n = out_n_start ;

	// iconv()
	//G_DEBUG( "G::ConvertImp::narrow: in...\n" << hexdump<16>(in_p_start,in_p_end) ) ;
	size_t rc = call( ::iconv , m , &in_p , &in_n , &out_p , &out_n ) ;
	const size_t e = static_cast<size_t>(ssize_t(-1)) ;
	if( rc == e || in_p != in_p_end || out_n > out_n_start )
		throw Convert::Error( "iconv failed" + std::string(context.empty()?"":": ") + context ) ;
	//G_DEBUG( "G::ConvertImp::narrow: out...\n" << G::hexdump<16>(out_p_start,out_p) ) ;

	return std::string( &out_buffer[0] , &out_buffer[0] + (out_n_start-out_n) ) ;
}

void G::ConvertImp::from_wstring( std::vector<char> & buffer , const std::wstring & s )
{
	// wchar_t is not necessarily two bytes, so do it long-hand
	buffer.reserve( s.size() + 2U ) ;
	buffer.push_back( 0xff ) ;
	buffer.push_back( 0xfe ) ;
	for( size_t i = 0U ; i < s.size() ; i++ )
	{
		unsigned int n = s.at(i) ;
		buffer.push_back( n & 0xff ) ;
		buffer.push_back( n >> 8U ) ;
	}
}

std::wstring G::ConvertImp::to_wstring( const std::vector<char> & buffer )
{
	// wchar_t is not necessarily two bytes, so do it long-hand
	std::wstring result ;
	result.reserve( buffer.size() ) ;
	const unsigned char * p = reinterpret_cast<const unsigned char*>(&buffer[0]) ;
	const unsigned char * end = p + buffer.size() ;
	bool bom_ff_fe = buffer.size() >= 2U && p[0] == 0xff && p[1] == 0xfe ;
	bool bom_fe_ff = buffer.size() >= 2U && p[0] == 0xfe && p[1] == 0xff ;
	if( bom_ff_fe || bom_fe_ff ) p += 2 ;
	int hi = bom_fe_ff ? 0 : 1 ;
	int lo = bom_fe_ff ? 1 : 0 ;
	for( ; (p+1) < end ; p += 2 )
	{
		wchar_t w = static_cast<wchar_t>( ( static_cast<unsigned int>(p[hi]) << 8 ) | p[lo] ) ;
		result.append( 1U , w ) ;
	}
	return result ;
}

G::ConvertImp & G::ConvertImp::utf16_to_utf8()
{
	static ConvertImp c( "UTF-8" , "UTF-16" ) ;
	return c ;
}

G::ConvertImp & G::ConvertImp::utf16_to_ansi()
{
	static ConvertImp c( "ISO-8859-15" , "UTF-16" ) ;
	return c ;
}

G::ConvertImp & G::ConvertImp::utf8_to_utf16()
{
	static ConvertImp c( "UTF-16" , "UTF-8" ) ;
	return c ;
}

G::ConvertImp & G::ConvertImp::ansi_to_utf16()
{
	static ConvertImp c( "UTF-16" , "ISO-8859-15" ) ;
	return c ;
}

// ==

std::wstring G::Convert::widen( const std::string & s , bool utf8 , const std::string & context )
{
	if( s.empty() )
	{
		return std::wstring() ;
	}
	else if( utf8 )
	{
		return ConvertImp::utf8_to_utf16().widen( s , context ) ;
	}
	else
	{
		return ConvertImp::ansi_to_utf16().widen( s , context ) ;
	}
}

std::string G::Convert::narrow( const std::wstring & s , bool utf8 , const std::string & context )
{
	if( s.empty() )
	{
		return std::string() ;
	}
	else if( utf8 )
	{
		return ConvertImp::utf16_to_utf8().narrow( s , context ) ;
	}
	else
	{
		return ConvertImp::utf16_to_ansi().narrow( s , context ) ;
	}
}

