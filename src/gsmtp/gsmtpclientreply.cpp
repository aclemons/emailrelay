//
// Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "glog.h"
#include "gassert.h"

GSmtp::ClientReply::ClientReply()
{
	G_ASSERT( !m_valid && !m_complete && m_value == 0 ) ;
}

GSmtp::ClientReply GSmtp::ClientReply::ok()
{
	ClientReply reply ;
	reply.m_valid = true ;
	reply.m_complete = true ;
	reply.m_value = 250 ;
	reply.m_text = "OK" ;
	return reply ;
}

GSmtp::ClientReply GSmtp::ClientReply::error( Value v , const std::string & response ,
	const std::string & reason )
{
	int vv = static_cast<int>(v) ;
	ClientReply reply ;
	reply.m_valid = true ;
	reply.m_complete = true ;
	reply.m_value = ( vv >= 500 && vv < 600 ) ? vv : 500 ;
	reply.m_text = G::Str::printable( response ) ;
	reply.m_reason = reason ;
	return reply ;
}

GSmtp::ClientReply GSmtp::ClientReply::start()
{
	ClientReply reply ;
	reply.m_valid = true ;
	reply.m_complete = true ;
	reply.m_value = static_cast<int>(Value::Internal_start) ;
	return reply ;
}

GSmtp::ClientReply GSmtp::ClientReply::ok( Value v , const std::string & text )
{
	ClientReply reply ;
	reply.m_valid = true ;
	reply.m_complete = true ;
	reply.m_value = static_cast<int>(v) ;
	reply.m_text = text.empty() ? "OK" : ("OK\n"+text) ;
	G_ASSERT( reply.positive() ) ;
	return reply ;
}

GSmtp::ClientReply::ClientReply( const G::StringArray & lines )
{
	G_ASSERT( !m_valid && !m_complete && m_value == 0 ) ;
	for( const auto & line : lines )
	{
		if( validLine(line) )
		{
			int n = G::Str::toInt( line.substr(0U,3U) ) ;
			if( n >= 100 && n <= 599 && ( m_value == 0 || m_value == n ) )
			{
				m_valid = true ;
				m_complete = line.length() == 3U || line.at(3U) == ' ' ;
				m_value = n ;
				if( line.length() > 4U )
				{
					std::string s = line.substr(4U) ;
					G::Str::trimLeft( s , " \t" ) ;
					G::Str::replaceAll( s , "\t" , " " ) ;
					if( !m_text.empty() && !s.empty() ) m_text.append(1U,'\n') ;
					m_text.append( s ) ;
				}
			}
			else
			{
				m_valid = false ;
				break ;
			}
		}
		else
		{
			m_valid = false ;
			break ;
		}
	}
	if( !m_valid )
	{
		m_complete = false ;
		m_value = 0 ;
		m_text.clear() ;
		m_reason.clear() ;
	}
}

bool GSmtp::ClientReply::validLine( const std::string & line )
{
	return
		line.length() >= 3U &&
		isDigit(line.at(0U)) &&
		line.at(0U) >= '1' && line.at(0U) <= '5' &&
		isDigit(line.at(1U)) &&
		isDigit(line.at(2U)) &&
		( line.length() == 3U || line.at(3U) == ' ' || line.at(3U) == '-' ) ;
}

bool GSmtp::ClientReply::valid() const
{
	return m_valid ;
}

bool GSmtp::ClientReply::complete() const
{
	return m_complete ;
}

bool GSmtp::ClientReply::incomplete() const
{
	return !m_complete ;
}

bool GSmtp::ClientReply::positive() const
{
	return m_valid && m_value < 400 ;
}

bool GSmtp::ClientReply::positiveCompletion() const
{
	return type() == Type::PositiveCompletion ;
}

int GSmtp::ClientReply::value() const
{
	return m_valid ? m_value : 0 ;
}

bool GSmtp::ClientReply::is( Value v ) const
{
	return value() == static_cast<int>(v) ;
}

std::string GSmtp::ClientReply::errorText() const
{
	return positiveCompletion() ? std::string() : ( m_text.empty() ? std::string("error") : m_text ) ;
}

std::string GSmtp::ClientReply::errorReason() const
{
	return m_reason ;
}

std::string GSmtp::ClientReply::text() const
{
	return m_text ;
}

bool GSmtp::ClientReply::isDigit( char c )
{
	return c >= '0' && c <= '9' ;
}

GSmtp::ClientReply::Type GSmtp::ClientReply::type() const
{
	G_ASSERT( m_valid && (m_value/100) >= 1 && (m_value/100) <= 5 ) ;
	return static_cast<Type>( m_value / 100 ) ;
}

GSmtp::ClientReply::SubType GSmtp::ClientReply::subType() const
{
	G_ASSERT( m_valid && m_value >= 0 ) ;
	int n = ( m_value / 10 ) % 10 ;
	if( n < 4 )
		return static_cast<SubType>( n ) ;
	else
		return SubType::Invalid_SubType ;
}


