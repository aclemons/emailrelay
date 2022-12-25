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

//| \class GSmtp::ClientReply
/// Encapsulates SMTP replies from a remote client, or replies from
/// a client filter, or the result of a TLS handshake.
///
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
		///< Constructor taking lines of text from the remote SMTP client.
		///< Typically there is only one line, but SMTP responses can be
		///< extended onto more than one line by using a dash after the
		///< result code.

	static ClientReply ok() ;
		///< Factory function returning a generic 'Ok' reply object.

	static ClientReply start() ;
		///< Factory function returning an 'Internal_start' reply object.

	static ClientReply ok( Value , const std::string & = std::string() ) ;
		///< Factory function returning a success reply object with the
		///< given Value.

	static ClientReply error( Value , const std::string & response , const std::string & error_reason ) ;
		///< Factory function returning an error reply object with
		///< the given Value (typically Internal_filter_error),
		///< client-filter response and client-filter reason.
		///< See also GSmtp::Filter.

	bool valid() const ;
		///< Returns true if syntactically valid.

	bool complete() const ;
		///< Returns true if the string array passed to the constructor
		///< is syntactically a complete reply.

	bool incomplete() const ;
		///< Returns !complete().

	bool positive() const ;
		///< Returns true if valid() and value() is less than 400.

	bool positiveCompletion() const ;
		///< Returns true if type() is PositiveCompletion.

	bool is( Value v ) const ;
		///< Returns true if the value() is as given.

	int value() const ;
		///< Returns the numeric value of the reply.

	std::string text() const ;
		///< Returns the text of the reply.

	std::string errorText() const ;
		///< Returns the non-empty error text of the reply or the
		///< empty string if positiveCompletion().

	std::string errorReason() const ;
		///< Returns the error() reason text of the reply.

	Type type() const ;
		///< Returns the type enumeration.

	SubType subType() const ;
		///< Returns the sub-type enumeration.

private:
	ClientReply() ;
	static bool isDigit( char ) ;
	static bool validLine( const std::string & line ) ;

private:
	bool m_complete {false} ;
	bool m_valid {false} ;
	int m_value {0} ;
	std::string m_text ;
	std::string m_reason ; // additional error reason
} ;

#endif
