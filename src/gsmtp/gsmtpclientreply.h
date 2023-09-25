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
	enum class Value
	{
		Invalid = 0 ,
		Internal_start = 1 ,
		Internal_filter_ok = 2 ,
		Internal_filter_abandon = 3 ,
		Internal_filter_error = 4 ,
		Internal_secure = 5 ,
		ServiceReady_220 = 220 ,
		Ok_250 = 250 ,
		Authenticated_235 = 235 ,
		Challenge_334 = 334 ,
		OkForData_354 = 354 ,
		SyntaxError_500 = 500 ,
		SyntaxError_501 = 501 ,
		NotImplemented_502 = 502 ,
		BadSequence_503 = 503 ,
		NotAuthenticated_535 = 535 ,
		NotAvailable_454 = 454
	} ;

	static bool valid( const G::StringArray & ) ;
		///< Returns true if the reply text is syntactivally valid
		///< but possibly incomplete.

	static bool complete( const G::StringArray & ) ;
		///< Returns true if the reply text is valid() and complete.

	explicit ClientReply( const G::StringArray & text , char sep = '\n' ) ;
		///< Constructor taking lines of text from the remote SMTP client.
		///< If there is more than one line in the SMTP response (eg. in
		///< the EHLO response) then the resulting text() value is a
		///< concatenation using the given separator.
		///<
		///< Precondition: complete(text)

	static ClientReply ok() ;
		///< Factory function returning a generic 'Ok' reply object
		///< with a value() of 250.

	static ClientReply secure() ;
		///< Factory function for Internal_secure.

	static ClientReply filterOk() ;
		///< Factory function for Internal_filter_ok.

	static ClientReply filterAbandon() ;
		///< Factory function for Internal_filter_abandon.

	static ClientReply filterError( const std::string & response , const std::string & filter_reason ) ;
		///< Factory function for Internal_filter_error.

	static ClientReply start() ;
		///< Factory function for Internal_start.

	bool positive() const ;
		///< Returns true if value() is between 100 and 399.

	bool positiveCompletion() const ;
		///< Returns true if value() is between 200 and 299.

	bool is( Value v ) const ;
		///< Returns true if the value() is as given.

	int value() const ;
		///< Returns the numeric value of the reply.

	std::string text() const ;
		///< Returns the text of the reply, with some whitespace
		///< normalisation and no tabs.

	int doneCode() const ;
		///< Returns -1 for filterAbandon() or -2 for filterError()
		///< or zero if less than 100 or value().
		/// \see GSmtp::ClientProtocol::doneSignal()

	std::string errorText() const ;
		///< Returns the empty string if positiveCompletion()
		///< or non-empty text() or "error".

	std::string reason() const ;
		///< Returns the filter-reason text from a filterError() reply
		///< or the empty string.

private:
	ClientReply() ;
	static bool isDigit( char ) ;
	static bool validLine( const std::string & line , std::string & , std::size_t , std::size_t ) ;
	static ClientReply internal( Value , int ) ;

private:
	int m_value {0} ;
	int m_done_code {0} ;
	std::string m_text ;
	std::string m_filter_reason ; // if Internal_filter_error
} ;

#endif
