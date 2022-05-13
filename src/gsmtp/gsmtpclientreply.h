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
/// \file gsmtpclientreply.h
///

#ifndef G_SMTP_CLIENT_REPLY_H
#define G_SMTP_CLIENT_REPLY_H

#include "gdef.h"
#include "gstringarray.h"
#include <string>

namespace GSmtp
{
	class ClientReply ;
}

class GSmtp::ClientReply
{
public:
	enum class Type
	{
		PositivePreliminary = 1 ,
		PositiveCompletion = 2 ,
		PositiveIntermediate = 3 ,
		TransientNegative = 4 ,
		PermanentNegative = 5
	} ;
	enum class SubType
	{
		Syntax = 0 ,
		Information = 1 ,
		Connections = 2 ,
		MailSystem = 3 ,
		Invalid_SubType = 4
	} ;
	enum class Value
	{
		Invalid = 0 ,
		Internal_start = 1 ,
		Internal_filter_ok = 2 ,
		Internal_filter_abandon = 3 ,
		Internal_secure = 4 ,
		ServiceReady_220 = 220 ,
		Ok_250 = 250 ,
		Authenticated_235 = 235 ,
		Challenge_334 = 334 ,
		OkForData_354 = 354 ,
		SyntaxError_500 = 500 ,
		SyntaxError_501 = 501 ,
		NotImplemented_502 = 502 ,
		BadSequence_503 = 503 ,
		Internal_filter_error = 590 ,
		NotAuthenticated_535 = 535 ,
		NotAvailable_454 = 454
	} ;

	explicit ClientReply( const G::StringArray & ) ;
	static ClientReply ok() ;
	static ClientReply start() ;
	static ClientReply ok( Value , const std::string & = std::string() ) ;
	static ClientReply error( Value , const std::string & response , const std::string & error_reason ) ;
	bool valid() const ;
	bool complete() const ;
	bool incomplete() const ;
	bool positive() const ;
	bool positiveCompletion() const ;
	bool is( Value v ) const ;
	int value() const ;
	std::string text() const ;
	std::string errorText() const ;
	std::string errorReason() const ;
	Type type() const ;
	SubType subType() const ;

private:
	ClientReply() ;
	static bool isDigit( char ) ;
	static bool validLine( const std::string & line ) ;

private:
	bool m_complete{false} ;
	bool m_valid{false} ;
	int m_value{0} ;
	std::string m_text ;
	std::string m_reason ; // additional error reason
} ;

#endif
