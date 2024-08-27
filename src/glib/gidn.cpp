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
/// \file gidn.cpp
///

#include "gdef.h"
#include "gidn.h"
#include "gstr.h"
#include "gconvert.h"
#include "gstringtoken.h"
#include "glog.h"
#include <vector>
#include <limits>
#include <iomanip>
#include <algorithm>
#include <iterator>

namespace G
{
	class IdnImp ;
}

class G::IdnImp
{
public:
	IdnImp() ;
	IdnImp & encode( std::string_view domain ) ;
	std::string result() const { return m_output ; }
	static bool is7Bit( std::string_view s ) noexcept ;

private:
	using unicode_type = G::Convert::unicode_type ;
	using value_type = g_uint32_t ;
	using List = std::vector<unicode_type> ;

private:
	void outputPunycode( std::string_view ) ;
	static bool parse( List& , unicode_type , std::size_t , std::size_t ) ;
	static value_type adapt( value_type d , value_type n , bool first ) noexcept ;
	struct div_t { value_type quot ; value_type rem ; } ;
	static div_t div( value_type numerator , value_type demoninator ) noexcept ;
	static value_type clamp( value_type v , value_type lo , value_type hi ) noexcept ;
	static void check( bool ) ;
	static void check( value_type , value_type , value_type ) ;
	static bool is7Bit_( char ) noexcept ;

private:
	static constexpr value_type c_skew = 38U ;
	static constexpr value_type c_damp = 700U ;
	static constexpr value_type c_base = 36U ;
	static constexpr value_type c_tmin = 1U ;
	static constexpr value_type c_tmax = 26U ;
	static constexpr value_type c_initial_bias = 72U ;
	static constexpr value_type c_initial_n = 128U ;
	std::string m_output ;
	List m_ulist ;
} ;

bool G::Idn::valid( std::string_view domain )
{
	// TODO full IDN validation

	if( domain.empty() || !G::Str::isPrintable(domain) )
		return false ;

	for( G::StringTokenView t(domain,".",1U) ; t ; ++t )
	{
		if( t().empty() || ( !G::Str::isPrintableAscii(t()) && !G::Convert::valid(t()) ) )
			return false ;
	}
	return true ;
}

std::string G::Idn::encode( std::string_view domain )
{
	if( domain.empty() || IdnImp::is7Bit(domain) )
		return G::sv_to_string( domain ) ;
	else
		return IdnImp().encode(domain).result() ;
}

// ==

G::IdnImp::IdnImp()
= default ;

G::IdnImp & G::IdnImp::encode( std::string_view domain )
{
	m_output.reserve( domain.size() * 2U ) ;
	m_ulist.reserve( domain.size() ) ;
	bool first = true ;
	for( G::StringTokenView t(domain,".",1U) ; t ; ++t , first = false )
	{
		m_output.append( first?0U:1U , '.' ) ;
		if( is7Bit(t()) )
		{
			m_output.append( t.data() , t.size() ) ;
		}
		else
		{
			m_output.append( "xn--" , 4U ) ;
			outputPunycode( t() ) ;
		}
	}
	return *this ;
}

void G::IdnImp::outputPunycode( std::string_view label )
{
	// RFC-3492 pseudocode transliteration...

	std::size_t b0 = m_output.size() ;
	std::copy_if( label.begin() , label.end() , std::back_inserter(m_output) , &IdnImp::is7Bit_ ) ;
	value_type b = static_cast<value_type>( m_output.size() - b0 ) ;
	if( b ) m_output.append( 1U , '-' ) ;

	using namespace std::placeholders ;
	G::Convert::u8parse( label , std::bind(&IdnImp::parse,std::ref(m_ulist),_1,_2,_3) ) ;

	static constexpr std::string_view c_map { "abcdefghijklmnopqrstuvwxyz0123456789" , 36U } ;
	value_type n = c_initial_n ;
	value_type delta = 0 ;
	value_type bias = c_initial_bias ;
	for( value_type h = b ; h < static_cast<value_type>(m_ulist.size()) ; delta++ , n++ )
	{
		auto m_p = std::min_element( m_ulist.begin() , m_ulist.end() ,
			[n](unicode_type a_,unicode_type b_){return (a_<n?0x110000U:a_) < (b_<n?0x110000U:b_) ;} ) ;

		G_ASSERT( m_p != m_ulist.end() && *m_p >= n ) ;
		check( m_p != m_ulist.end() && *m_p >= n ) ;
		G_DEBUG( "idn: next code point is " << std::hex << std::setfill('0') << std::setw(4U) << *m_p ) ;
		check( delta , *m_p-n , h+1U ) ;

		delta += (*m_p-n) * (h+1U) ;
		n = *m_p ;
		for( std::size_t i = 0U ; i < m_ulist.size() ; i++ ) // NOLINT modernize-loop-convert
		{
			if( m_ulist[i] < n ) { delta++ ; check( delta != 0U ) ; }
			if( m_ulist[i] == n )
			{
				auto q = delta ;
				const auto output_size = m_output.size() ; GDEF_IGNORE_VARIABLE(output_size) ; // NOLINT
				for( value_type k = c_base ;; k += c_base )
				{
					value_type t = clamp( k-std::min(k,bias) , c_tmin , c_tmax ) ; // 1..26
					if( q < t ) break ;
					auto x = div( q-t , c_base-t ) ; static_assert(c_base>c_tmax,"") ;
					q = x.quot ;
					m_output.push_back( c_map.at(std::size_t(t)+x.rem) ) ;
				}
				m_output.push_back( c_map.at(q) ) ;
				G_DEBUG( "idn: delta " << delta << ", encodes as \"" << m_output.substr(output_size) << "\"" ) ;
				bias = adapt( delta , h+1U , h == b ) ;
				G_DEBUG( "idn: bias becomes " << bias ) ;
				delta = 0 ;
				++h ;
			}
		}
	}
}

G::IdnImp::value_type G::IdnImp::adapt( value_type d , value_type n , bool first ) noexcept
{
	d /= ( first ? c_damp : 2U ) ;
	d += ( d / n ) ;
	value_type k = 0U ;
	for( ; d > ((c_base-c_tmin)*c_tmax)/2 ; k += c_base )
		d /= (c_base-c_tmin) ;
	return k + ((c_base-c_tmin+1U)*d) / (d+c_skew) ;
}

bool G::IdnImp::parse( List & output , unicode_type u , std::size_t , std::size_t )
{
	output.push_back( {u} ) ;
	return true ;
}

G::IdnImp::value_type G::IdnImp::clamp( value_type v , value_type lo , value_type hi ) noexcept
{
	// std::clamp() is c++17
	return v < lo ? lo : ( hi < v ? hi : v ) ;
}

bool G::IdnImp::is7Bit_( char c ) noexcept
{
	return ( static_cast<unsigned char>(c) & 0x80U ) == 0U ;
}

bool G::IdnImp::is7Bit( std::string_view s ) noexcept
{
	return std::all_of( s.begin() , s.end() , &IdnImp::is7Bit_ ) ;
}

G::IdnImp::div_t G::IdnImp::div( value_type top , value_type bottom ) noexcept
{
	// cf. std::div()
	return { top/bottom , top%bottom } ;
}

void G::IdnImp::check( bool b )
{
	if( !b )
		throw Idn::Error() ; // never gets here
}

void G::IdnImp::check( value_type a , value_type b , value_type c )
{
	constexpr value_type maxint = std::numeric_limits<value_type>::max() ;
	if( c != 0U && b > (maxint-a)/c )
		throw Idn::Error( "domain name too long: numeric overflow multiplying by " + std::to_string(c) ) ;
}

