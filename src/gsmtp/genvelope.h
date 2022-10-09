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
/// \file genvelope.h
///

#ifndef G_SMTP_ENVELOPE_H
#define G_SMTP_ENVELOPE_H

#include "gdef.h"
#include "gstringarray.h"
#include "gexception.h"
#include <iostream>

namespace GSmtp
{
	class Envelope ;
}

//| \class GSmtp::Envelope
/// A structure containing the contents of an envelope file, with
/// support for file reading, writing and copying.
///
class GSmtp::Envelope
{
public:
	G_EXCEPTION( ReadError , tx("cannot read envelope file") ) ;

public:
	static void read( std::istream & , Envelope & ) ;
		///< Reads an envelope from a stream. Throws on error.
		///< Input lines can be newline delimited, in which case
		///< 'm_crlf' is set false.

	static std::size_t write( std::ostream & , const Envelope & ) ;
		///< Writes an envelope to a stream. Returns the new endpos
		///< value. Returns zero on error. Output lines are CR-LF
		///< delimited. The structure 'm_crlf' and 'm_endpos' fields
		///< should normally be updated after using write().

	static void copy( std::istream & , std::ostream & ) ;
		///< A convenience function to copy lines from an input
		///< stream to an output stream. Input lines can be newline
		///< delimited, but output is always CR-LF. Throws on input
		///< error; output errors are not checked.

public:
	bool m_crlf{true} ;
	int m_eight_bit{-1} ;
	std::string m_from ;
	G::StringArray m_to_local ;
	G::StringArray m_to_remote ;
	std::string m_authentication ;
	std::string m_client_socket_address ;
	std::string m_client_certificate ;
	std::string m_from_auth_in ;
	std::string m_from_auth_out ;
	std::size_t m_endpos{0U} ;
} ;

#endif
