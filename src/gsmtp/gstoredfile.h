//
// Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gstoredfile.h
///

#ifndef G_SMTP_STORED_FILE__H
#define G_SMTP_STORED_FILE__H

#include "gdef.h"
#include "gmessagestore.h"
#include "gstoredmessage.h"
#include "gexception.h"
#include "gpath.h"
#include "gstrings.h"
#include <iostream>
#include <memory>

namespace GSmtp
{
	class StoredFile ;
}

/// \class GSmtp::StoredFile
/// A concete derived class implementing the
/// StoredMessage interface.
///
class GSmtp::StoredFile : public StoredMessage
{
public:
	G_EXCEPTION( FormatError , "invalid envelope file" ) ;
	G_EXCEPTION( FilenameError , "invalid envelope filename" ) ;
	G_EXCEPTION( ReadError , "cannot read envelope file" ) ;

	StoredFile( FileStore & store , const G::Path & envelope_path ) ;
		///< Constructor.

	virtual ~StoredFile() ;
		///< Destructor. Unlocks the file if it has been lock()ed
		///< but not destroy()ed or fail()ed.

	bool lock() ;
		///< Locks the file by renaming the envelope file.
		///< Used by FileStore and FileIterator.

	bool readEnvelope( std::string & reason , bool check_for_no_remote_recipients ) ;
		///< Reads the envelope. Returns false on error.
		///< Used by FileStore and FileIterator.

	bool openContent( std::string & reason ) ;
		///< Opens the content file. Returns false on error.
		///< Used by FileStore and FileIterator.

	virtual std::string name() const override ;
		///< Override from GSmtp::StoredMessage.

	virtual void fail( const std::string & reason , int reason_code ) override ;
		///< Override from GSmtp::StoredMessage.

private: // overrides
	virtual std::string location() const override ; // Override from GSmtp::StoredMessage.
	virtual int eightBit() const override ; // Override from GSmtp::StoredMessage.
	virtual std::string from() const override ; // Override from GSmtp::StoredMessage.
	virtual std::string to( size_t ) const override ; // Override from GSmtp::StoredMessage.
	virtual size_t toCount() const override ; // Override from GSmtp::StoredMessage.
	virtual std::string authentication() const override ; // Override from GSmtp::StoredMessage.
	virtual std::string fromAuthIn() const override ; // Override from GSmtp::StoredMessage.
	virtual std::string fromAuthOut() const override ; // Override from GSmtp::StoredMessage.
	virtual void close() override ; // Override from GSmtp::StoredMessage.
	virtual std::string reopen() override ; // Override from GSmtp::StoredMessage.
	virtual void destroy() override ; // Override from GSmtp::StoredMessage.
	virtual void unfail() override ; // Override from GSmtp::StoredMessage.
	virtual std::istream & contentStream() override ; // Override from GSmtp::StoredMessage.
	virtual size_t errorCount() const override ; // Override from GSmtp::StoredMessage.

private:
	StoredFile( const StoredFile & ) g__eq_delete ;
	void operator=( const StoredFile & ) g__eq_delete ;
	const std::string & eol() const ;
	G::Path contentPath() const ;
	std::string readLine( std::istream & ) ;
	std::string readValue( std::istream & , const std::string & key ) ;
	std::string value( const std::string & ) const ;
	void readFormat( std::istream & stream ) ;
	void readFlag( std::istream & stream ) ;
	void readFrom( std::istream & stream ) ;
	void readFromAuthIn( std::istream & stream ) ;
	void readFromAuthOut( std::istream & stream ) ;
	void readToList( std::istream & stream ) ;
	void readEnd( std::istream & stream ) ;
	void readReasons( std::istream & stream ) ;
	void readAuthentication( std::istream & stream ) ;
	void readClientSocketAddress( std::istream & stream ) ;
	void readClientSocketName( std::istream & stream ) ;
	void readClientCertificate( std::istream & stream ) ;
	void readEnvelopeCore( bool ) ;
	void addReason( const G::Path & path , const std::string & , int ) const ;
	static G::Path badPath( const G::Path & ) ;

private:
	FileStore & m_store ;
	unique_ptr<std::istream> m_content ;
	G::StringArray m_to_local ;
	G::StringArray m_to_remote ;
	std::string m_from ;
	std::string m_from_auth_in ;
	std::string m_from_auth_out ;
	G::Path m_envelope_path ;
	G::Path m_old_envelope_path ;
	std::string m_name ;
	int m_eight_bit ;
	std::string m_authentication ;
	std::string m_format ;
	std::string m_client_socket_address ;
	std::string m_client_socket_name ;
	std::string m_client_certificate ;
	size_t m_errors ;
	bool m_locked ;
	bool m_crlf ;
} ;

#endif

