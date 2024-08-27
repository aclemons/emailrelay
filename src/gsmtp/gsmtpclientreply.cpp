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
/// \file gsmtpclientreply.cpp
///

#include "gdef.h"
#include "gsmtpclientreply.h"
#include "gstr.h"
#include "gstringarray.h"
#include "gexception.h"
#include "glog.h"
#include "gassert.h"

GSmtp::ClientReply::ClientReply()
= default ;

#ifndef G_LIB_SMALL
GSmtp::ClientReply GSmtp::ClientReply::ok()
{
	ClientReply reply ;
	reply.m_value = 250 ;
	reply.m_done_code = 250 ;
	reply.m_text = "OK" ;
	G_ASSERT( reply.positive() ) ;
	return reply ;
}
#endif

GSmtp::ClientReply GSmtp::ClientReply::internal( Value v , int done_code )
{
	G_ASSERT( static_cast<int>(v) >= 1 && static_cast<int>(v) < 100 ) ;
	ClientReply reply ;
	reply.m_value = static_cast<int>( v ) ;
	reply.m_done_code = done_code ;
	return reply ;
}

GSmtp::ClientReply GSmtp::ClientReply::secure()
{
	return internal( Value::Internal_secure , 0 ) ;
}

GSmtp::ClientReply GSmtp::ClientReply::start()
{
	return internal( Value::Internal_start , 0 ) ;
}

GSmtp::ClientReply GSmtp::ClientReply::filterOk()
{
	return internal( Value::Internal_filter_ok , 0 ) ;
}

GSmtp::ClientReply GSmtp::ClientReply::filterAbandon()
{
	return internal( Value::Internal_filter_abandon , -1 ) ;
}

GSmtp::ClientReply GSmtp::ClientReply::filterError( const std::string & response ,
	const std::string & filter_reason )
{
	ClientReply reply ;
	reply.m_value = static_cast<int>( Value::Internal_filter_error ) ;
	reply.m_done_code = -2 ;
	reply.m_text = response ;
	reply.m_filter_reason = filter_reason ;
	return reply ;
}

GSmtp::ClientReply::ClientReply( const G::StringArray & lines , char sep )
{
	G_ASSERT( complete(lines) ) ;
	if( !complete(lines) )
		throw G::Exception( "invalid client response" ) ;

	m_value = G::Str::toInt( lines.at(lines.size()-1U).substr(0U,3U) ) ;
	m_done_code = m_value ;
	G_ASSERT( m_value >= 100 && m_value < 600 ) ;

	for( const auto & line : lines )
	{
		if( line.length() > 4U )
		{
			std::string s = line.substr( 4U ) ;
			G::Str::trimLeft( s , " \t" ) ;
			G::Str::replace( s , '\t' , ' ' ) ;
			G::Str::replace( s , '\n' , ' ' ) ;
			G::Str::removeAll( s , '\r' ) ;
			if( !m_text.empty() && !s.empty() ) m_text.append(1U,sep) ;
			m_text.append( s ) ;
		}
	}
}

bool GSmtp::ClientReply::valid( const G::StringArray & lines )
{
	if( lines.empty() )
		return false ;

	std::string digits ;
	for( std::size_t i = 0U ; i < lines.size() ; i++ )
	{
		if( !validLine(lines[i],digits,i,lines.size()) )
			return false ;
	}
	return true ;
}

bool GSmtp::ClientReply::complete( const G::StringArray & lines )
{
	if( lines.empty() ) return false ;
	const std::string & last = lines[lines.size()-1U] ;
	return valid(lines) && ( last.size() == 3U || last.at(3U) == ' ' ) ;
}

bool GSmtp::ClientReply::validLine( const std::string & line , std::string & digits ,
	std::size_t index , std::size_t )
{
	if( index == 0U && line.length() >= 3U )
		digits = line.substr( 0U , 3U ) ;
	else if( index == 0U )
		return false ;

	return
		line.length() >= 3U &&
		isDigit(line.at(0U)) &&
		line.at(0U) >= '1' && line.at(0U) <= '5' &&
		isDigit(line.at(1U)) &&
		isDigit(line.at(2U)) &&
		digits == line.substr(0U,3U) &&
		( line.length() == 3U || ( line.at(3U) == ' ' || line.at(3U) == '-' ) ) ;
}

bool GSmtp::ClientReply::isDigit( char c )
{
	return c >= '0' && c <= '9' ;
}

bool GSmtp::ClientReply::positive() const
{
	return m_value >= 100 && m_value < 400 ;
}

bool GSmtp::ClientReply::positiveCompletion() const
{
	return m_value >= 200 && m_value < 300 ;
}

int GSmtp::ClientReply::value() const
{
	return m_value ;
}

bool GSmtp::ClientReply::is( Value v ) const
{
	return m_value == static_cast<int>(v) ;
}

int GSmtp::ClientReply::doneCode() const
{
	return m_done_code ;
}

std::string GSmtp::ClientReply::errorText() const
{
	return positiveCompletion() ? std::string() : ( m_text.empty() ? std::string("error") : m_text ) ;
}

std::string GSmtp::ClientReply::reason() const
{
	return m_filter_reason ;
}

std::string GSmtp::ClientReply::text() const
{
	return m_text ;
}

