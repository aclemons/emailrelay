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
/// \file gstringwrap.cpp
///

#include "gdef.h"
#include "gstringwrap.h"
#include "gimembuf.h"
#include "gstr.h"
#include <string>
#include <sstream>
#include <algorithm>
#include <clocale>
#include <locale>

#ifdef emit
#undef emit
#endif

struct G::StringWrap::Config /// Private implementation structure for G::StringWrap.
{
	string_view prefix_first ;
	string_view prefix_other ;
	std::size_t width_first ;
	std::size_t width_other ;
	bool preserve_spaces ;
} ;

class G::StringWrap::WordWrapper /// Private implementation structure for G::StringWrap.
{
public:
	WordWrapper( std::ostream & , StringWrap::Config , const std::locale & ) ;
	void emit( const std::string & word , std::size_t newlines , const std::string & prespace ) ;

public:
	~WordWrapper() = default ;
	WordWrapper( const WordWrapper & ) = delete ;
	WordWrapper( WordWrapper && ) = delete ;
	WordWrapper & operator=( const WordWrapper & ) = delete ;
	WordWrapper & operator=( WordWrapper && ) = delete ;

private:
	string_view prefix() const ;

private:
	std::size_t m_lines {0U} ;
	std::size_t m_w {0U} ;
	std::ostream & m_out ;
	StringWrap::Config m_config ;
	const std::locale & m_loc ;
} ;

// ==

G::StringWrap::WordWrapper::WordWrapper( std::ostream & out , Config config , const std::locale & loc ) :
	m_out(out) ,
	m_config(config) ,
	m_loc(loc)
{
}

G::string_view G::StringWrap::WordWrapper::prefix() const
{
	return m_lines ? m_config.prefix_other : m_config.prefix_first ;
}

void G::StringWrap::WordWrapper::emit( const std::string & word , std::size_t newlines , const std::string & prespace )
{
	// emit words up to the configured maximum width
	std::size_t wordsize = StringWrap::wordsize( word , m_loc ) ;
	std::size_t spacesize = m_config.preserve_spaces ? prespace.size() : 1U ;
	std::size_t width = ( newlines || m_lines > 1 ) ? m_config.width_other : m_config.width_first ;
	bool start_new_line = newlines || m_w == 0 || (m_w+spacesize+wordsize) > width ;
	if( start_new_line )
	{
		// emit a blank line for each of the counted newlines
		bool first_line = m_w == 0 ;
		for( std::size_t i = 1U ; i < newlines ; i++ )
		{
			m_out << (first_line?"":"\n") << prefix() ;
			first_line = false ;
			m_w = prefix().size() ;
			m_lines++ ;
		}

		// emit the first word
		m_out << (first_line?"":"\n") << prefix() << word ;
		m_w = prefix().size() + wordsize ;
		m_lines++ ;
	}
	else
	{
		if( m_config.preserve_spaces )
			m_out << (prespace.empty()?std::string(1U,' '):prespace) << word ;
		else
			m_out << " " << word ;
		m_w += (spacesize+wordsize) ;
	}
}

// ==

std::string G::StringWrap::wrap( const std::string & text_in ,
	const std::string & prefix_first , const std::string & prefix_other ,
	std::size_t width_first , std::size_t width_other ,
	bool preserve_spaces , const std::locale & loc )
{
	StringWrap::Config config {
		prefix_first ,
		prefix_other ,
		width_first , width_other?width_other:width_first ,
		preserve_spaces
	} ;

	std::ostringstream out ;
	WordWrapper wrapper( out , config , loc ) ;

	imembuf buf( text_in.data() , text_in.size() ) ;
	std::istream in( &buf ) ;
	in.imbue( loc ) ;

	wrap( in , wrapper ) ;

	return out.str() ;
}

void G::StringWrap::wrap( std::istream & in , WordWrapper & ww )
{
	// extract words while counting newlines within the intervening spaces
	const auto & cctype = std::use_facet<std::ctype<char>>( in.getloc() ) ;
	std::string word ;
	std::size_t newlines = 0U ;
	std::string prespace ;
	char c = 0 ;
	while( in.get(c) )
	{
		if( cctype.is( std::ctype_base::space , c ) )
		{
			// spit out the previous word (if any)
			if( !word.empty() )
			{
				ww.emit( word , newlines , prespace ) ;
				newlines = 0U ;
				prespace.clear() ;
			}

			// start the new word
			word.clear() ;
			if( c == '\n' )
			{
				newlines++ ;
				prespace.clear() ;
			}
			else
			{
				prespace.append( 1U , c ) ;
			}
		}
		else
		{
			word.append( 1U , c ) ;
		}
	}
	// spit out the trailing word (if any)
	if( !word.empty() )
		ww.emit( word , newlines , prespace ) ;
}

std::locale G::StringWrap::defaultLocale()
{
	try
	{
		// see also G::gettext_init()
		return std::locale( std::setlocale(LC_CTYPE,nullptr) ) ;
	}
	catch( std::exception & )
	{
		return std::locale::classic() ;
	}
}

std::size_t G::StringWrap::wordsize( const std::string & s , const std::locale & loc )
{
	return wordsize( string_view(s.data(),s.size()) , loc ) ;
}

std::size_t G::StringWrap::wordsize( G::string_view s , const std::locale & loc )
{
	try
	{
		// convert the input to 32-bit wide characters using codecvt::in() and count
		// them -- this would be a lot easier if the character set was known to be either
		// ISO-8859 or UTF-8, but in principle it could be some unknown MBCS set
		// from the environment -- errors are ignored because returning the
		// input string length is a good fallback -- note that the 'classic' locale
		// will result in conversion errors if any character is more than 127
		const auto & codecvt = std::use_facet<std::codecvt<char32_t,char,std::mbstate_t>>( loc ) ;
		std::vector<char32_t> warray( s.size() ) ; // 'internal', in()'s 'to'
		std::mbstate_t state {} ;
		const char * cnext = nullptr ;
		char32_t * wnext = nullptr ;
		auto rc = codecvt.in( state ,
			s.data() , s.end() , cnext ,
			warray.data() , warray.data()+warray.size() , wnext ) ;
		std::size_t din = cnext ? std::distance( s.data() , cnext ) : 0U ;
		std::size_t dout = wnext ? std::distance( warray.data() , wnext ) : 0U ;
		return ( rc == std::codecvt_base::ok && din == s.size() && dout ) ? dout : s.size() ;
	}
	catch( std::exception & )
	{
		return s.size() ;
	}
}

