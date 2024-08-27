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
/// \file submitparser.cpp
///

#include "gdef.h"
#include "submitparser.h"
#include "gconvert.h"
#include "gidn.h"
#include "gbase64.h"
#include "gstr.h"
#include "gassert.h"
#include "glog.h"
#include <stdexcept>
#include <algorithm>
#include <cassert>

namespace SubmitParser
{
	bool is_ctext( const char * p , std::size_t n ) noexcept
	{
		unsigned int c = static_cast<unsigned char>(*p) ;
		return
			n > 1U || // RFC-6532
			( c >= 33U && c <= 39U ) ||
			( c >= 42U && c <= 91U ) ||
			( c >= 93U && c <= 126U ) ;
	}
	bool is_vchar( const char * p , std::size_t n ) noexcept
	{
		// RFC-5234 visible characters with RFC-6532 UTF-8
		unsigned int c = static_cast<unsigned char>(*p) ;
		return
			n > 1U || // RFC-6532
			( c >= 0x21U && c <= 0xFEU ) ;
	}
	bool is_atext( const char * p , std::size_t n ) noexcept
	{
		char c = *p ;
		return
			n > 1U || // RFC-6532
			( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) ||
			( c >= '0' && c <= '9' ) ||
			c == '!' || c == '#' ||
			c == '$' || c == '%' ||
			c == '&' || c == '\'' ||
			c == '*' || c == '+' ||
			c == '-' || c == '/' ||
			c == '=' || c == '?' ||
			c == '^' || c == '_' ||
			c == '`' || c == '{' ||
			c == '|' || c == '}' ||
			c == '~' ;
	}
	bool is_qtext( const char * p , std::size_t n ) noexcept
	{
		unsigned int c = static_cast<unsigned char>(*p) ;
		return
			n > 1U || // RFC-6532
			c == 33U ||
			( c >= 35U && c <= 91U ) ||
			( c >= 93U && c <= 126U ) ;
	}
	template <typename Tp> void assert_( bool (* /*fn*/ )(Tp,Tp) , Tp /*begin*/ , Tp /*end*/ )
	{
		//G_ASSERT( (*fn)(begin,end) ) ;
	}
	bool is_ws( char c ) noexcept
	{
		return c == ' ' || c == '\t' ;
	}
	constexpr bool is_word( const Token & t ) noexcept
	{
		return t.first == T::atom || t.first == T::dot_atom || t.first == T::quote ;
	}
	constexpr bool is_atom( const Token & t ) noexcept
	{
		return t.first == T::atom || t.first == T::dot_atom ;
	}
	constexpr bool is_char( const Token & t , char c ) noexcept
	{
		return t.first == T::character && !t.second.empty() && t.second[0] == c ;
	}
	constexpr bool is_cfws( const Token & t ) noexcept
	{
		return t.first == T::ws || t.first == T::comment ;
	}
	template <typename Tp> Tp skip_cfws( Tp begin , Tp end )
	{
		return std::find_if( begin , end , [](const typename Tp::value_type &t){return !is_cfws(t);} ) ;
	}
	template <typename Tp> Tp skip_display_name( Tp begin , Tp end )
	{
		return std::find_if( begin , end , [](const typename Tp::value_type &t){return !is_word(t);} ) ;
	}
	template <typename Tp> Tp read_display_name( Tp begin , Tp end , std::string & out )
	{
		Tp last = std::find_if( begin , end , [](const typename Tp::value_type &t){return !is_word(t);} ) ;
		for( Tp p = begin ; p != last ; ++p )
			out.append(out.empty()?0U:1U,' ').append((*p).second) ;
		return last ;
	}
	template <typename Tp> std::string str( Tp begin , Tp end ) // for debugging
	{
		std::string result( 1U , '{' ) ;
		for( Tp p = begin ; p != end ; ++p )
			result
				.append(result.size()==1U?0U:1U,'|')
				.append(std::to_string(static_cast<int>((*p).first)))
				.append(1U,'=')
				.append((*p).second) ;
		result.append( 1U , '}' ) ;
		return result ;
	}
	Mailbox make_mailbox( const std::string & local_part , const std::string & domain_part , const std::string & display_name = {} )
	{
		return {local_part,domain_part,display_name} ;
	}
	template <typename Tp> bool is_angle_addr( Tp begin , Tp end )
	{
		Tp p = skip_cfws( begin , end ) ;
		return
			std::distance(p,end) >= 5U &&
			is_char(*p,'<') &&
			is_addr_spec( p+1 , p+4 ) &&
			is_char(*(p+4),'>') &&
			( std::distance(p,end) == 5U ||
				std::all_of( p+5 , end , [](const typename Tp::value_type &t_){return is_cfws(t_);} ) ) ;
	}
	template <typename Tp> bool is_addr_spec( Tp begin , Tp end )
	{
		return
			std::distance(begin,end) == 3U &&
			is_word(*begin) &&
			is_char(*(begin+1),'@') &&
			is_atom(*(begin+2)) ;
	}
	template <typename Tp> Mailbox parse_addr_spec( Tp begin , Tp end )
	{
		assert_( is_addr_spec , begin , end ) ;
		return make_mailbox( (*begin).second , (*(begin+2)).second ) ;
	}
	template <typename Tp> bool is_name_addr( Tp begin , Tp end )
	{
		return is_angle_addr( skip_display_name(begin,end) , end ) ;
	}
	template <typename Tp> Mailbox parse_name_addr( Tp begin , Tp end )
	{
		assert_( is_name_addr , begin , end ) ;
		std::string display_name ;
		Tp p = read_display_name( begin , end , display_name ) ;
		return make_mailbox( (*(p+1)).second , (*(p+3)).second , display_name ) ;
	}
	template <typename Tp> bool is_mailbox( Tp begin , Tp end )
	{
		bool addr_spec = is_addr_spec( begin , end ) ;
		bool name_addr = is_name_addr( begin , end ) ;
		return addr_spec || name_addr ;
	}
	template <typename Tp> Mailbox parse_mailbox( Tp begin , Tp end )
	{
		assert_( is_mailbox , begin , end ) ;
		if( is_addr_spec( begin , end ) )
			return parse_addr_spec( begin , end ) ;
		else if( is_name_addr( begin , end ) )
			return parse_name_addr( begin , end ) ;
		else
			return {} ; // never gets here
	}
	template <typename Tp> Tp next( Tp p , Tp end )
	{
		return p == end ? end : std::next(p) ;
	}
	template <typename Tp> Tp next2( Tp p , Tp end )
	{
		if( p == end ) return end ;
		if( std::next(p) == end ) return end ;
		return std::next(std::next(p)) ;
	}
	template <typename Tp> bool is_mailbox_list( Tp begin , Tp end )
	{
		if( begin == end ) return false ;
		auto last = begin ;
		for( auto first = begin ; first != end ; first = next(last,end) )
		{
			last = std::find_if( first , end , [](const typename Tp::value_type & t_){return is_char(t_,',');} ) ;
			if( is_addr_spec( first , last ) || is_name_addr( first , last ) )
				;
			else
				return false ;
		}
		return true ;
	}
	template <typename Tp, typename Op> void parse_mailbox_list( Tp begin , Tp end , Op op )
	{
		assert_( is_mailbox_list , begin , end ) ;
		auto last = begin ;
		for( auto first = begin ; first != end ; first = next(last,end) )
		{
			last = std::find_if( first , end , [](const typename Tp::value_type & t_){return is_char(t_,',');} ) ;
			if( is_addr_spec( first , last ) )
				op( parse_addr_spec( first , last ) ) ;
			else if( is_name_addr( first , last ) )
				op( parse_name_addr( first , last ) ) ;
		}
	}
	template <typename Tp> Tp skip_group( Tp begin , Tp end )
	{
		auto colon = std::find_if( begin , end , [](const typename Tp::value_type& t_){return is_char(t_,':');} ) ;
		auto semi = std::find_if( next(colon,end) , end , [](const typename Tp::value_type& t_){return is_char(t_,';');} ) ;
		bool is_group =
			begin != end && colon != end && semi != end &&
			begin != colon && // display-name not empty
			std::all_of( begin , colon , [](const typename Tp::value_type& t_){return is_word(t_);} ) && // display-name
			( std::next(colon) == semi || // group-list is optional
				( is_cfws(*std::next(colon)) && next2(colon,end) == semi ) || // group-list is cfws
				is_mailbox_list(std::next(colon),semi) ) ; // group-list is mailbox-list
		return is_group ? skip_cfws(std::next(semi),end) : begin ;
	}
	template <typename Tp> bool starts_with_group( Tp begin , Tp end )
	{
		return skip_group( begin , end ) != begin ;
	}
	template <typename Tp> bool is_group( Tp begin , Tp end )
	{
		return skip_group( begin , end ) == end ;
	}
	template <typename Tp, typename Op> Tp parse_group( Tp begin , Tp end , Op op )
	{
		assert_( starts_with_group , begin , end ) ;
		auto colon = std::find_if( begin , end , [](const typename Tp::value_type& t_){return is_char(t_,':');} ) ;
		auto semi = std::find_if( next(colon,end) , end , [](const typename Tp::value_type& t_){return is_char(t_,';');} ) ;
		if( is_mailbox_list( std::next(colon) , semi ) )
		{
			parse_mailbox_list( std::next(colon) , semi , op ) ;
		}
		return skip_cfws( std::next(semi) , end ) ;
	}
	template <typename Tp> bool is_address( Tp begin , Tp end )
	{
		if( begin == end ) return false ;
		return
			is_group( begin , end ) ||
			is_mailbox( begin , end ) ;
	}
	template <typename Tp, typename Op> bool parse_address( Tp begin , Tp end , Op op )
	{
		assert_( is_address , begin , end ) ;
		if( is_group( begin , end ) )
		{
			Tp next = parse_group( begin , end , [](const Mailbox &){} ) ;
			if( next != end )
				return false ; // more than one address
			parse_group( begin , end , op ) ;
		}
		else if( is_mailbox( begin , end ) )
		{
			op( parse_mailbox( begin , end ) ) ;
		}
		return true ;
	}
	template <typename Tp> bool is_address_list( Tp begin , Tp end )
	{
		if( begin == end ) return false ;
		auto last = end ;
		for( auto first = begin ; first != end ; first = next(last,end) )
		{
			if( starts_with_group( first , end ) ) // display-name : [group-list] ; [cfws]
			{
				last = skip_group( first , end ) ;
			}
			else
			{
				last = std::find_if( first , end , [](const typename Tp::value_type & t_){return is_char(t_,',');} ) ;
				if( !is_addr_spec( first , last ) && !is_name_addr( first , last ) )
				{
					return false ;
				}
			}
		}
		return true ;
	}
	template <typename Tp, typename Op> void parse_address_list( Tp begin , Tp end , Op op )
	{
		assert_( is_address_list , begin , end ) ;
		auto last = end ;
		for( auto first = begin ; first != end ; first = next(last,end) )
		{
			if( starts_with_group( first , end ) )
			{
				last = parse_group( first , end , op ) ;
			}
			else
			{
				last = std::find_if( first , end , [](const typename Tp::value_type & t_){return is_char(t_,',');} ) ;
				if( is_addr_spec( first , last ) )
					op( parse_addr_spec( first , last ) ) ;
				else if( is_name_addr( first , last ) )
					op( parse_name_addr( first , last ) ) ;
			}
		}
	}
	std::string join_for_content( const Mailbox & mbox )
	{
		// keep it simple by only returning in name-addr format if
		// there are no funny characters in the display name
		const std::string & display_name = std::get<2>(mbox) ;
		if( !display_name.empty() && G::Str::isPrintable(display_name) && display_name.find_first_of("\\\"\t") == std::string::npos )
			return "\"" + display_name + "\" <" + std::get<0>(mbox) + "@" + std::get<1>(mbox) + ">" ; // name-addr
		else
			return std::get<0>(mbox) + "@" + std::get<1>(mbox) ; // addr-spec
	}
	std::string join_for_envelope( const Mailbox & mbox , std::string_view error_more )
	{
		if( !G::Idn::valid(std::get<1>(mbox)) )
			throw Error( "invalid domain encoding [" + G::Str::printable(std::get<1>(mbox)) + "]" , error_more ) ;
		return std::get<0>(mbox) + "@" + G::Idn::encode(std::get<1>(mbox)) ;
	}
	std::vector<Token> lexImp( std::string_view ) ;
	void check( const std::vector<Token> & , std::string_view ) ;
	void elide( std::vector<Token> & ) ;
	void elideImp( std::vector<Token> & , T major_a , T major_b , T minor_a , T minor_b ) ;
}

// ==

std::string SubmitParser::parseMailbox( std::string_view line , std::string_view error_more )
{
	auto tokens = lex( line , error_more ) ;
	if( !is_mailbox( tokens.cbegin() , tokens.cend() ) )
		throw Error( "invalid mailbox" , error_more ) ;
	return join_for_envelope( parse_mailbox( tokens.cbegin() , tokens.cend() ) , error_more ) ;
}

void SubmitParser::parseMailboxList( std::string_view line , G::StringArray & out , std::string_view error_more )
{
	auto tokens = lex( line , error_more ) ;
	if( !is_mailbox_list( tokens.cbegin() , tokens.cend() ) )
		throw Error( "invalid mailbox-list" , error_more ) ;
	parse_mailbox_list( tokens.cbegin() , tokens.cend() , [&out,error_more](const Mailbox &mbox){out.push_back(join_for_envelope(mbox,error_more));} ) ;
}

void SubmitParser::parseAddress( std::string_view line , G::StringArray & out , std::string_view error_more )
{
	auto tokens = lex( line , error_more ) ;
	if( !is_address( tokens.cbegin() , tokens.cend() ) )
		throw Error( "invalid address" , error_more ) ;
	if( !parse_address( tokens.cbegin() , tokens.cend() , [&out,error_more](const Mailbox & mbox){out.push_back(join_for_envelope(mbox,error_more));} ) )
		throw Error( "invalid address: too many parts" , error_more ) ;
}

void SubmitParser::parseAddressList( std::string_view line , G::StringArray & out , bool as_content , std::string_view error_more )
{
	auto tokens = lex( line , error_more ) ;
	if( !is_address_list( tokens.cbegin() , tokens.cend() ) )
		throw Error( "invalid address-list" , error_more ) ;
	if( as_content )
		parse_address_list( tokens.cbegin() , tokens.cend() , [&out](const Mailbox & mbox){out.push_back(join_for_content(mbox));} ) ;
	else
		parse_address_list( tokens.cbegin() , tokens.cend() , [&out,error_more](const Mailbox & mbox){out.push_back(join_for_envelope(mbox,error_more));} ) ;
}

void SubmitParser::check( const std::vector<Token> & tokens , std::string_view error_more )
{
	if( tokens.size() == 1U && tokens[0].first == T::error )
		throw Error( "parsing error at position " + std::to_string(tokens[0].second.size()) , error_more ) ;
}

void SubmitParser::elide( std::vector<Token> & tokens )
{
	elideImp( tokens , T::atom , T::dot_atom , T::comment , T::ws ) ; // atom = [CFWS] 1*atext [CFWS]
	elideImp( tokens , T::quote , T::quote , T::comment , T::ws ) ; // quoted-string = [CFWS] DQUOTE ... DQUOTE [CFWS]
}

void SubmitParser::elideImp( std::vector<Token> & tokens , T major_a , T major_b , T minor_a , T minor_b )
{
	for( std::size_t i = 0U ; i < tokens.size() ; i++ )
	{
		if( tokens[i].first == major_a || tokens[i].first == major_b )
		{
			for( std::size_t j = i-1U ; j < tokens.size() ; )
			{
				if( tokens[j].first == minor_a || tokens[j].first == minor_b )
					tokens[j].first = T::error ;
				else
					break ;
				if( j == 0U ) break ; else // for ms static analyser
				j-- ;
			}
			for( std::size_t j = i+1U ; j < tokens.size() ; j++ )
			{
				if( tokens[j].first == minor_a || tokens[j].first == minor_b )
					tokens[j].first = T::error ;
				else
					break ;
			}
		}
	}
	tokens.erase( std::remove_if( tokens.begin() , tokens.end() ,
		[](const Token & t_){return t_.first == T::error;} ) , tokens.end() ) ;
}

std::vector<SubmitParser::Token> SubmitParser::lex( std::string_view line , std::string_view error_more )
{
	auto tokens = lexImp( line ) ;
	G_DEBUG( "SubmitParser::lex: " << G::Str::printable(str(tokens.cbegin(),tokens.cend())) ) ;
	check( tokens , error_more ) ;
	elide( tokens ) ;
	return tokens ;
}

std::vector<SubmitParser::Token> SubmitParser::lexImp( std::string_view line )
{
	// RFC-5322 3.2 ...
	std::vector<Token> out ;
	enum class State
	{
		idle ,
		atom ,
		quote ,
		comment ,
		ws ,
		error ,
	} ;
	State state = State::idle ;
	int depth = 0 ;
	const char * end = line.data() + line.size() ;
	bool qp = false ;
	for( const char * p = line.data() ; p != end ; qp = !qp && *p=='\\' , ++p )
	{
		auto pair = G::Convert::u8in( reinterpret_cast<const unsigned char*>(p) , std::distance(p,end) ) ;
		if( pair.first == G::Convert::unicode_error || pair.second < 1U )
			state = State::error ;
		std::size_t n = pair.second ;

		if( state == State::error )
		{
			out.clear() ;
			out.emplace_back( T::error , std::string(line.data(),p-line.data()) ) ;
			break ;
		}
		else if( state == State::idle )
		{
			if( *p == '"' )
			{
				state = State::quote ;
				out.emplace_back( T::quote , std::string() ) ;
			}
			else if( *p == '(' )
			{
				state = State::comment ;
				out.emplace_back( T::comment , std::string() ) ;
				depth = 1 ;
			}
			else if( is_atext(p,n) )
			{
				state = State::atom ;
				out.emplace_back( T::atom , std::string(p,n) ) ;
				p += (n-1U) ;
			}
			else if( is_ws(*p) )
			{
				state = State::ws ;
				out.emplace_back( T::ws , std::string(1U,' ') ) ;
			}
			else if( *p == '@' || *p == ':' || *p == ';' || *p == ',' || *p == '<' || *p == '>' )
			{
				out.emplace_back( T::character , std::string(1U,*p) ) ;
			}
			else
			{
				state = State::error ;
			}
		}
		else if( state == State::quote )
		{
			if( qp && ( is_vchar(p,n) || is_ws(*p) ) )
			{
				// qcontent is quoted-pair , quoted-pair = ("\" (VCHAR / WSP))
				out.back().second.append( p , n ) ;
				p += (n-1U) ;
			}
			else if( qp )
			{
				state = State::error ;
			}
			else if( *p == '"' )
			{
				state = State::idle ;
			}
			else if( *p == '\\' )
			{
				// "invisible"
			}
			else if( is_qtext(p,n) )
			{
				// qcontent is qtext
				out.back().second.append( p , n ) ;
				p += (n-1U) ;
			}
			else if( is_ws(*p) )
			{
				// quoted-string = ...*([FWS] qcontent)...
				out.back().second.append( 1U , *p ) ;
			}
			else
			{
				state = State::error ;
			}
		}
		else if( state == State::comment )
		{
			if( *p == ')' && !qp && depth == 1 )
			{
				state = State::idle ;
				depth = 0 ;
			}
			else if( *p == ')' && !qp )
			{
				out.back().second.append( 1U , *p ) ;
				--depth ;
			}
			else if( *p == '(' && !qp )
			{
				out.back().second.append( 1U , *p ) ;
				++depth ;
			}
			else if( *p == '\\' && !qp )
			{
				// "invisible"
			}
			else if( is_ws(*p) || is_ctext(p,n) )
			{
				out.back().second.append( p , n ) ;
				p += (n-1U) ;
			}
			else
			{
				state = State::error ;
			}
		}
		else if( state == State::atom )
		{
			if( *p == '.' )
			{
				out.back().first = T::dot_atom ;
				out.back().second.append( 1U , *p ) ;
			}
			else if( is_atext(p,n) )
			{
				out.back().second.append( p , n ) ;
				p += (n-1U) ;
			}
			else
			{
				state = State::idle ;
				p-- ;
			}
		}
		else if( state == State::ws )
		{
			if( is_ws(*p) )
			{
				// "invisible"
			}
			else
			{
				state = State::idle ;
				p-- ;
			}
		}
		else
		{
			state = State::error ; // never gets here
		}
	}
	return out ;
}

