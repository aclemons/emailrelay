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
/// \file gsmtpservertext.cpp
///

#include "gdef.h"
#include "gsmtpservertext.h"
#include "gdate.h"
#include "gtime.h"
#include "gdatetime.h"
#include "gstr.h"
#include "glog.h"
#include "gassert.h"
#include <string>

GSmtp::ServerText::ServerText( const std::string & code_ident , const std::string & thishost ,
	const GNet::Address & peer_address ) :
		m_code_ident(code_ident) ,
		m_thishost(thishost) ,
		m_peer_address(peer_address)
{
}

std::string GSmtp::ServerText::greeting() const
{
	return m_thishost + " -- " + m_code_ident + " -- Service ready" ;
}

std::string GSmtp::ServerText::hello( const std::string & ) const
{
	return m_thishost + " says hello" ;
}

std::string GSmtp::ServerText::received( const std::string & smtp_peer_name ,
	bool authenticated , bool secure , const std::string & protocol , const std::string & cipher ) const
{
	return receivedLine( smtp_peer_name , m_peer_address.hostPartString() , m_thishost ,
		authenticated , secure , protocol , cipher ) ;
}

std::string GSmtp::ServerText::receivedLine( const std::string & smtp_peer_name ,
	const std::string & peer_address , const std::string & thishost ,
	bool authenticated , bool secure , const std::string & , const std::string & cipher_in ) const
{
	const G::SystemTime t = G::SystemTime::now() ;
	const G::BrokenDownTime tm = t.local() ;
	const std::string zone = G::DateTime::offsetString(G::DateTime::offset(t)) ;
	const G::Date date( tm ) ;
	const G::Time time( tm ) ;
	const std::string esmtp = std::string("ESMTP") + (secure?"S":"") + (authenticated?"A":"") ; // RFC-3848
	const std::string peer_name = G::Str::toPrintableAscii(
		G::Str::replaced(smtp_peer_name,' ','-') ) ; // typically alphanumeric with ".-:[]_"
	std::string cipher = secure ?
		G::Str::only(G::Str::alnum_(),G::Str::replaced(cipher_in,'-','_')) :
		std::string() ;

	// RFC-5321 4.4
	std::ostringstream ss ;
	ss
		<< "Received: from " << peer_name
		<< " ("
			<< "[" << peer_address << "]"
		<< ") by " << thishost << " with " << esmtp
		<< (cipher.empty()?"":" tls ") << cipher // RFC-8314 4.3 7.4
		<< " ; "
		<< date.weekdayName(true) << ", "
		<< date.monthday() << " "
		<< date.monthName(true) << " "
		<< date.yyyy() << " "
		<< time.hhmmss(":") << " "
		<< zone ;
	return ss.str() ;
}

