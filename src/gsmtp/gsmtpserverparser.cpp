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
/// \file gsmtpserverparser.cpp
///

#include "gdef.h"
#include "gsmtpserverparser.h"
#include "gidn.h"
#include "gxtext.h"
#include "gstr.h"
#include "gstringtoken.h"
#include "gconvert.h"
#include "glog.h"
#include "gassert.h"
#include <string>

std::pair<std::size_t,bool> GSmtp::ServerParser::parseBdatSize( std::string_view bdat_line )
{
	G::StringTokenView token( bdat_line , "\t "_sv ) ;
	std::size_t size = 0U ;
	bool ok = false ;
	if( ++token )
	{
		bool overflow = false ;
		bool invalid = false ;
		std::size_t n = G::Str::toUnsigned<std::size_t>( token.data() , token.data()+token.size() , overflow , invalid ) ;
		if( !overflow && !invalid )
			size = n , ok = true ;
	}
	return {size,ok} ;
}

std::pair<bool,bool> GSmtp::ServerParser::parseBdatLast( std::string_view bdat_line )
{
	G::StringTokenView token( bdat_line , "\t "_sv ) ;
	bool last = false ;
	bool ok = false ;
	if( ++token )
	{
		ok = true ;
		if( ++token )
			ok = last = G::Str::imatch( "LAST"_sv , token() ) ;
	}
	return {last,ok} ;
}

GSmtp::ServerParser::AddressCommand GSmtp::ServerParser::parseMailFrom( std::string_view line , const Config & config )
{
	G::StringTokenView t( line , " \t"_sv ) ;
	if( !G::Str::imatch("MAIL"_sv,t()) || G::Str::ifind(t.next()(),"FROM:"_sv) != 0U )
		return {"invalid mail-from command"} ;

	AddressCommand result = parseAddressPart( line , config ) ;
	if( result.error.empty() )
	{
		if( !parseMailStringValue(line,"SMTPUTF8="_sv,result).empty() ) // RFC-6531 3.4 para1, but not clear
			result.error = "invalid mail-from parameter" ;

		result.auth = parseMailStringValue( line , "AUTH="_sv , result , Conversion::ValidXtext ) ;
		result.body = parseMailStringValue( line , "BODY="_sv , result , Conversion::Upper ) ; // RFC-1652, RFC-3030
		result.size = parseMailNumericValue( line , "SIZE="_sv , result ) ; // RFC-1427 submitter's size estimate
		result.smtputf8 = parseMailBoolean( line , "SMTPUTF8"_sv , result ) ;
		G_DEBUG( "GSmtp::ServerParser::parseMailFrom: error=[" << G::Str::printable(result.error) << "]" ) ;
		G_DEBUG( "GSmtp::ServerParser::parseMailFrom: address=[" << G::Str::printable(result.address) << "]" ) ;
		G_DEBUG( "GSmtp::ServerParser::parseMailFrom: size=" << result.size ) ;
		G_DEBUG( "GSmtp::ServerParser::parseMailFrom: auth=[" << G::Str::printable(result.auth) << "]" ) ;
		G_DEBUG( "GSmtp::ServerParser::parseMailFrom: smtputf8=" << (result.smtputf8?"1":"0") ) ;
	}
	return result ;
}

GSmtp::ServerParser::AddressCommand GSmtp::ServerParser::parseRcptTo( std::string_view line , const Config & config )
{
	G::StringTokenView t( line , " \t"_sv ) ;
	if( !G::Str::imatch("RCPT"_sv,t()) || G::Str::ifind(t.next()(),"TO:"_sv) != 0U )
		return {"invalid rcpt-to command"} ;

	return parseAddressPart( line , config ) ;
}

GSmtp::ServerParser::AddressCommand GSmtp::ServerParser::parseAddressPart( std::string_view line , const Config & config )
{
	// RFC-5321 4.1.2
	// eg. MAIL FROM:<>
	// eg. MAIL FROM:<me@localhost> SIZE=12345
	// eg. RCPT TO:<Postmaster>
	// eg. RCPT TO:<@first.net,@second.net:you@last.net>
	// eg. RCPT TO:<"alice\ \"jones\" :->"@example.com> XFOO=xyz

	// early check of the character-set to reject NUL and CR-LF
	if( line.find('\0') != std::string::npos ||
		line.find_first_of("\r\n",0,2U) != std::string::npos )
	{
		return {"invalid character in mailbox name"} ;
	}

	// find one past the colon
	std::size_t startpos = line.find( ':' ) ;
	if( startpos == std::string::npos )
		return {"missing colon"} ;
	startpos++ ;

	// test for possibly-allowed errors
	AddressCommand result ;
	if( startpos < line.size() && ( line[startpos] == ' ' || line[startpos] == '\t' ) )
		result.invalid_spaces = true ;
	startpos = line.find_first_not_of( " \t" , startpos , 2U ) ;
	if( startpos == std::string::npos ) startpos = line.size() ;
	if( startpos < line.size() && line[startpos] != '<' )
		result.invalid_nobrackets = true ;

	// fail unallowed errors
	if( result.invalid_spaces && !config.allow_spaces )
	{
		result.error = "invalid space after colon" ;
		return result ;
	}
	if( result.invalid_nobrackets && !config.allow_nobrackets )
	{
		result.error = "missing angle brackets in mailbox name" ;
		return result ;
	}

	// find the address part
	std::size_t endpos = 0U ;
	if( result.invalid_nobrackets )
	{
		endpos = line.find_first_of( " \t"_sv , startpos ) ;
		if( endpos == std::string::npos ) endpos = line.size() ;
		G_ASSERT( startpos < line.size() && endpos <= line.size() && endpos > startpos ) ;
	}
	else if( (startpos+2U) > line.size() || line.find('>',startpos+1U) == std::string::npos )
	{
		result.error = "invalid angle brackets in mailbox name" ;
		return result ;
	}
	else
	{
		// step over any source route so startpos is the colon not the "<"
		if( line.at(startpos+1U) == '@' )
		{
			// RFC-6531 complicates the syntax, but we follow RFC-5321 4.1.2 in
			// assuming there is no colon within the RFC-6531 A-d-l syntax element
			startpos = line.find( ':' , startpos+1U ) ;
			if( startpos == std::string::npos || (startpos+2U) >= line.size() )
				return {"invalid source route in mailbox name"} ;
		}

		// find the endpos allowing for quoted angle brackets and escaped quotes
		if( line.at(startpos+1U) == '"' )
		{
			for( std::size_t i = startpos+2U ; endpos == 0U && i < line.size() ; i++ )
			{
				if( line[i] == '\\' )
					i++ ;
				else if( line[i] == '"' )
					endpos = line.find( '>' , i ) ;
			}
			if( endpos == std::string::npos )
				return {"invalid quoting"} ;
		}
		else
		{
			endpos = line.find( '>' , startpos+1U ) ;
			G_ASSERT( endpos != std::string::npos ) ;
		}
		if( (endpos+1U) < line.size() && line.at(endpos+1U) != ' ' )
			return {"invalid angle brackets"} ;

		G_ASSERT( startpos < line.size() && endpos < line.size() && endpos > startpos ) ;
		G_ASSERT( line.at(startpos) == '<' || line.at(startpos) == ':' ) ;
		G_ASSERT( line.at(endpos) == '>' ) ;
	}

	std::string_view address =
		result.invalid_nobrackets ?
			std::string_view( line.data()+startpos , endpos-startpos ) :
			std::string_view( line.data()+startpos+1U , endpos-startpos-1U ) ;

	auto address_style = GStore::MessageStore::addressStyle( address ) ;
	if( address_style == AddressStyle::Invalid )
		return {"invalid character in mailbox name"} ;

	result.utf8_mailbox_part = address_style == AddressStyle::Utf8Both || address_style == AddressStyle::Utf8Mailbox ;
	result.utf8_domain_part = address_style == AddressStyle::Utf8Both || address_style == AddressStyle::Utf8Domain ;
	result.raw_address = G::sv_to_string( address ) ;
	result.address = result.utf8_domain_part ? encodeDomain(address) : result.raw_address ;
	result.address_style = address_style ;
	result.tailpos = result.invalid_nobrackets ? endpos : (endpos+1U) ;
	return result ;
}

std::string GSmtp::ServerParser::encodeDomain( std::string_view address )
{
	std::size_t at_pos = address.rfind( '@' ) ;
	std::string_view user = G::Str::headView( address , at_pos , address ) ;
	std::string_view domain = G::Str::tailView( address , at_pos ) ;
	return
		domain.empty() ?
			G::sv_to_string( address ) :
			G::sv_to_string(user).append(1U,'@').append(G::Idn::encode(domain)) ;
}

std::size_t GSmtp::ServerParser::parseMailNumericValue( std::string_view line , std::string_view key_eq , AddressCommand & out )
{
	std::size_t result = 0U ;
	if( out.error.empty() && out.tailpos != std::string::npos && out.tailpos < line.size() )
	{
		std::string str = parseMailStringValue( line , key_eq , out ) ;
		if( !str.empty() && G::Str::isULong(str) )
			result = static_cast<std::size_t>( G::Str::toULong(str,G::Str::Limited()) ) ;
	}
	return result ;
}

std::string GSmtp::ServerParser::parseMailStringValue( std::string_view line , std::string_view key_eq , AddressCommand & out , Conversion conversion )
{
	std::string result ;
	if( out.error.empty() && out.tailpos != std::string::npos && out.tailpos < line.size() )
	{
		std::string_view tail = G::sv_substr_noexcept( std::string_view(line) , out.tailpos ) ;
		G::StringTokenView word( tail , " \t"_sv ) ;
		for( ; word ; ++word )
		{
			if( G::Str::ifind( word() , key_eq ) == 0U && word().size() > key_eq.size() )
			{
				result = G::sv_to_string( word().substr(key_eq.size() ) ) ;
				break ;
			}
		}
		if( conversion == Conversion::ValidXtext )
			result = G::Xtext::encode( G::Xtext::decode(result) ) ; // ensure valid xtext
		else if( conversion == Conversion::Upper )
			result = G::Str::upper( result ) ;
	}
	return result ;
}

bool GSmtp::ServerParser::parseMailBoolean( std::string_view line , std::string_view key , AddressCommand & out )
{
	bool result = false ;
	if( out.error.empty() && out.tailpos != std::string::npos && out.tailpos < line.size() )
	{
		std::string_view tail = G::sv_substr_noexcept( line , out.tailpos ) ;
		G::StringTokenView word( tail , " \t"_sv ) ;
		for( ; word && !result ; ++word )
		{
			if( word() == key )
				result = true ;
		}
	}
	return result ;
}

std::string GSmtp::ServerParser::parseVrfy( const std::string & line_in )
{
	G_ASSERT( G::Str::ifind(line_in,"VRFY") == 0U ) ;
	std::string line = line_in ;
	G::Str::trimRight( line , {" \t",2U} ) ;

	if( line.size() > 9U )
	{
		// RFC-6531 3.7.4.2
		std::string tail = line.substr( line.size() - 9U ) ;
		G::Str::trimLeft( tail , {" \t",2U} ) ;
		if( G::Str::imatch( "SMTPUTF8"_sv , tail ) )
			line = line.substr( 0U , line.size()-9U ) ;
	}

	std::string to ;
	std::size_t pos = line.find_first_of( " \t" ) ;
	if( pos != std::string::npos )
		to = line.substr(pos) ;
	return G::Str::trimmed( to , {" \t",2U} ) ;
}

std::string GSmtp::ServerParser::parseHeloPeerName( const std::string & line )
{
	std::size_t pos = line.find_first_not_of( " \t" ) ;
	if( pos == std::string::npos )
		return {} ;

	pos = line.find_first_of( " \t" , pos ) ;
	if( pos == std::string::npos )
		return {} ;

	std::string smtp_peer_name = line.substr( pos + 1U ) ;
	G::Str::trim( smtp_peer_name , {" \t",2U} ) ;
	return smtp_peer_name ;
}

