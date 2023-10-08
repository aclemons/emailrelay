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
/// \file gsmtpserverparser.cpp
///

#include "gdef.h"
#include "gsmtpserverparser.h"
#include "gxtext.h"
#include "gstr.h"
#include "gstringtoken.h"
#include "glog.h"
#include "gassert.h"
#include <string>

GSmtp::ServerParser::MailboxStyle GSmtp::ServerParser::mailboxStyle( const std::string & mailbox )
{
	static constexpr const char * cc =
		"\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F"
		"\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F" "\x7F" ;

	bool invalid = mailbox.find_first_of( cc , 0U , 33U ) != std::string::npos ;
	bool ascii = !invalid && G::Str::isPrintableAscii( mailbox ) ;

	if( invalid )
		return MailboxStyle::Invalid ;
	else if( ascii )
		return MailboxStyle::Ascii ;
	else
		return MailboxStyle::Utf8 ;
}

std::pair<std::size_t,bool> GSmtp::ServerParser::parseBdatSize( G::string_view bdat_line )
{
	G::StringTokenView token( bdat_line , "\t "_sv ) ;
	std::size_t size = 0U ;
	bool ok = false ;
	if( token && ++token )
	{
		bool overflow = false ;
		bool invalid = false ;
		std::size_t n = G::Str::toUnsigned<std::size_t>( token.data() , token.data()+token.size() , overflow , invalid ) ;
		if( !overflow && !invalid )
			size = n , ok = true ;
	}
	return {size,ok} ;
}

std::pair<bool,bool> GSmtp::ServerParser::parseBdatLast( G::string_view bdat_line )
{
	G::StringTokenView token( bdat_line , "\t "_sv ) ;
	bool last = false ;
	bool ok = false ;
	if( token && ++token )
	{
		ok = true ;
		if( ++token )
			ok = last = G::Str::imatch( "LAST"_sv , token() ) ;
	}
	return {last,ok} ;
}

GSmtp::ServerParser::AddressCommand GSmtp::ServerParser::parseMailFrom( G::string_view line )
{
	G::StringTokenView t( line , " \t"_sv ) ;
	if( !G::Str::imatch("MAIL"_sv,t()) || G::Str::ifind(t.next()(),"FROM:"_sv) != 0U )
		return {"invalid mail-from command"} ;

	AddressCommand result = parseAddressPart( line ) ;
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

GSmtp::ServerParser::AddressCommand GSmtp::ServerParser::parseRcptTo( G::string_view line )
{
	G::StringTokenView t( line , " \t"_sv ) ;
	if( !G::Str::imatch("RCPT"_sv,t()) || G::Str::ifind(t.next()(),"TO:"_sv) != 0U )
		return {"invalid rcpt-to command"} ;

	return parseAddressPart( line ) ;
}

GSmtp::ServerParser::AddressCommand GSmtp::ServerParser::parseAddressPart( G::string_view line )
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

	// find the opening angle bracket
	std::size_t startpos = line.find( ':' ) ;
	if( startpos == std::string::npos )
		return {"missing colon"} ;
	startpos++ ;
	while( startpos < line.size() && line[startpos] == ' ' )
		startpos++ ; // (as requested)
	if( (startpos+2U) > line.size() || line[startpos] != '<' || line.find('>',startpos+1U) == std::string::npos )
	{
		return {"missing or invalid angle brackets in mailbox name"} ;
	}

	// step over any source route
	if( line[startpos+1U] == '@' )
	{
		// RFC-6531 complicates the syntax, but we follow RFC-5321 4.1.2 in
		// assuming there is no colon within the RFC-6531 A-d-l syntax element
		startpos = line.find( ':' , startpos+1U ) ;
		if( startpos == std::string::npos || (startpos+2U) >= line.size() )
			return {"invalid source route in mailbox name"} ;
	}

	// find the end, allowing for quoted angle brackets and escaped quotes
	std::size_t endpos = 0U ;
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

	G_ASSERT( startpos != std::string::npos && endpos != std::string::npos ) ;
	G_ASSERT( endpos > startpos ) ;
	G_ASSERT( endpos < line.size() ) ;
	G_ASSERT( line.at(startpos) == '<' || line.at(startpos) == ':' ) ;
	G_ASSERT( line.at(endpos) == '>' ) ;

	std::string address = std::string( line.data()+startpos+1U , endpos-startpos-1U ) ;

	auto style = mailboxStyle( address ) ;
	if( style == MailboxStyle::Invalid )
		return {"invalid character in mailbox name"} ;

	AddressCommand result ;
	result.address = std::string( line.data()+startpos+1U , endpos-startpos-1U ) ;
	result.utf8address = style == MailboxStyle::Utf8 ;
	result.tailpos = endpos+1U ;
	return result ;
}

std::size_t GSmtp::ServerParser::parseMailNumericValue( G::string_view line , G::string_view key_eq , AddressCommand & out )
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

std::string GSmtp::ServerParser::parseMailStringValue( G::string_view line , G::string_view key_eq , AddressCommand & out , Conversion conversion )
{
	std::string result ;
	if( out.error.empty() && out.tailpos != std::string::npos && out.tailpos < line.size() )
	{
		G::string_view tail = G::sv_substr( G::string_view(line) , out.tailpos ) ;
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

bool GSmtp::ServerParser::parseMailBoolean( G::string_view line , G::string_view key , AddressCommand & out )
{
	bool result = false ;
	if( out.error.empty() && out.tailpos != std::string::npos && out.tailpos < line.size() )
	{
		G::string_view tail = G::sv_substr( line , out.tailpos ) ;
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
		return std::string() ;

	pos = line.find_first_of( " \t" , pos ) ;
	if( pos == std::string::npos )
		return std::string() ;

	std::string smtp_peer_name = line.substr( pos + 1U ) ;
	G::Str::trim( smtp_peer_name , {" \t",2U} ) ;
	return smtp_peer_name ;
}

