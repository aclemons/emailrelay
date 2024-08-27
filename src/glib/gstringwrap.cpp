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
/// \file gstringwrap.cpp
///

#include "gdef.h"
#include "gstringwrap.h"
#include "gimembuf.h"
#include <string>
#include <sstream>
#include <algorithm>

#ifdef emit
#undef emit
#endif

namespace G
{
	namespace StringWrapImp
	{
		struct Config
		{
			std::string_view prefix_first ;
			std::string_view prefix_other ;
			std::size_t width_first ;
			std::size_t width_other ;
			bool preserve_spaces ;
		} ;
		class WordWrapper
		{
		public:
			WordWrapper( std::ostream & , Config ) ;
			void emit( const std::string & word , std::size_t newlines , const std::string & prespace ) ;

		public:
			~WordWrapper() = default ;
			WordWrapper( const WordWrapper & ) = delete ;
			WordWrapper( WordWrapper && ) = delete ;
			WordWrapper & operator=( const WordWrapper & ) = delete ;
			WordWrapper & operator=( WordWrapper && ) = delete ;

		private:
			std::string_view prefix() const ;

		private:
			std::size_t m_lines {0U} ;
			std::size_t m_w {0U} ;
			std::ostream & m_out ;
			Config m_config ;
		} ;
		void wrapImp( std::istream & , WordWrapper & ) ;
	}
}

// ==

G::StringWrapImp::WordWrapper::WordWrapper( std::ostream & out , Config config ) :
	m_out(out) ,
	m_config(config)
{
}

std::string_view G::StringWrapImp::WordWrapper::prefix() const
{
	return m_lines ? m_config.prefix_other : m_config.prefix_first ;
}

void G::StringWrapImp::WordWrapper::emit( const std::string & word , std::size_t newlines , const std::string & prespace )
{
	// emit words up to the configured maximum width
	std::size_t wordsize = StringWrap::wordsize( word ) ;
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
	bool preserve_spaces )
{
	StringWrapImp::Config config {
		prefix_first ,
		prefix_other ,
		width_first , width_other?width_other:width_first ,
		preserve_spaces
	} ;

	std::ostringstream out ;
	StringWrapImp::WordWrapper wrapper( out , config ) ;

	imembuf buf( text_in.data() , text_in.size() ) ;
	std::istream in( &buf ) ;

	StringWrapImp::wrapImp( in , wrapper ) ;

	return out.str() ;
}

void G::StringWrapImp::wrapImp( std::istream & in , WordWrapper & ww )
{
	// extract words while counting newlines within the intervening spaces
	std::string word ;
	std::size_t newlines = 0U ;
	std::string prespace ;
	char c = 0 ;
	while( in.get(c) )
	{
		if( c == ' ' || c == '\n' )
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

std::size_t G::StringWrap::wordsize( const std::string & s )
{
	// (do the UTF-8 parsing ourselves to avoid a dependency on G::Convert::u8parse())
	const unsigned char * p = reinterpret_cast<const unsigned char*>( s.data() ) ;
	std::size_t n = s.size() ;
	std::size_t result = 0U ;
	std::size_t d = 0U ;
	for( std::size_t i = 0U ; i < n ; i += d , p += d )
	{
		result++ ;
		if( ( p[0] & 0x80U ) == 0U )
		{
			d = 1U ;
		}
		else if( ( p[0] & 0xE0U ) == 0xC0U && (i+1U) < n &&
			( p[1] & 0xC0 ) == 0x80U )
		{
			d = 2U ;
		}
		else if( ( p[0] & 0xF0U ) == 0xE0U && (i+2U) < n &&
			( p[1] & 0xC0 ) == 0x80U &&
			( p[2] & 0xC0 ) == 0x80U )
		{
			d = 3U ;
		}
		else if( ( p[0] & 0xF8U ) == 0xF0U && (i+3U) < n &&
			( p[1] & 0xC0 ) == 0x80U &&
			( p[2] & 0xC0 ) == 0x80U &&
			( p[3] & 0xC0 ) == 0x80U )
		{
			d = 4U ;
		}
		else
		{
			d = 1U ;
		}
	}
	return result ;
}

