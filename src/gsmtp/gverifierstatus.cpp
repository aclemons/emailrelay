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
/// \file gverifierstatus.cpp
///

#include "gdef.h"
#include "gverifier.h"
#include "gverifierstatus.h"
#include "gsmtpserverparser.h"
#include "gstringview.h"
#include "gstr.h"
#include "glog.h"

GSmtp::VerifierStatus::VerifierStatus()
= default;

GSmtp::VerifierStatus GSmtp::VerifierStatus::invalid( const std::string & recipient ,
	bool temporary , const std::string & response , const std::string & reason )
{
	VerifierStatus status ;
	status.is_valid = false ;
	status.temporary = temporary ;
	status.recipient = recipient ;
	status.response = response ;
	status.reason = reason ;
	return status ;
}

GSmtp::VerifierStatus GSmtp::VerifierStatus::remote( const std::string & recipient ,
	const std::string & address )
{
	VerifierStatus status ;
	status.is_valid = true ;
	status.is_local = false ;
	status.recipient = recipient ;
	status.address = address.empty() ? recipient : address ;
	return status ;
}

GSmtp::VerifierStatus GSmtp::VerifierStatus::local( const std::string & recipient ,
	const std::string & full_name , const std::string & mbox )
{
	VerifierStatus status ;
	status.is_valid = true ;
	status.is_local = true ;
	status.recipient = recipient ;
	status.full_name = full_name ;
	status.address = mbox ;
	return status ;
}

GSmtp::VerifierStatus GSmtp::VerifierStatus::parse( const std::string & line )
{
	try
	{
		G::StringArray part ;
		G::Str::splitIntoFields( line , part , '|' , '\\' ) ;
		if( part.size() != 9U )
			throw InvalidStatus() ;

		std::size_t i = 0U ;
		VerifierStatus s ;
		s.recipient = part.at(i++) ;
		s.is_valid = part.at(i++) == "1" ;
		s.is_local = part.at(i++) == "1" ;
		s.temporary = part.at(i++) == "1" ;
		s.abort = part.at(i++) == "1" ;
		s.full_name = part.at(i++) ;
		s.address = part.at(i++) ;
		s.response = part.at(i++) ;
		s.reason = part.at(i++) ;
		return s ;
	}
	catch( std::exception & )
	{
		G_ERROR( "GSmtp::VerifierStatus::parse: invalid verifier status: [" << line << "]" ) ;
		throw ;
	}
}

std::string GSmtp::VerifierStatus::str() const
{
	auto escape = [](const std::string &s){ return G::Str::escaped( s , '\\' , "\\|"_sv , "\\|"_sv ) ; } ;
	const char sep = '|' ;
	const char t = '1' ;
	const char f = '0' ;
	return escape(recipient)
		.append(1U,sep)
		.append(1U,is_valid?t:f).append(1U,sep)
		.append(1U,is_local?t:f).append(1U,sep)
		.append(1U,temporary?t:f).append(1U,sep)
		.append(1U,abort?t:f).append(1U,sep)
		.append(escape(full_name)).append(1U,sep)
		.append(escape(address)).append(1U,sep)
		.append(escape(response)).append(1U,sep)
		.append(escape(reason)) ;
}

bool GSmtp::VerifierStatus::utf8address() const
{
	return ServerParser::mailboxStyle( address ) == ServerParser::MailboxStyle::Utf8 ;
}

