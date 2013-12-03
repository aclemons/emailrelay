//
// Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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

#ifndef G_SMTP_STORED_FILE_H
#define G_SMTP_STORED_FILE_H

#include "gdef.h"
#include "gsmtp.h"
#include "gmessagestore.h"
#include "gexecutable.h"
#include "gstoredmessage.h"
#include "gexception.h"
#include "gpath.h"
#include "gstrings.h"
#include <iostream>
#include <memory>

/// \namespace GSmtp
namespace GSmtp
{
	class StoredFile ;
}

/// \class GSmtp::StoredFile
/// A concete derived class implementing the
/// StoredMessage interface.
///
class GSmtp::StoredFile : public GSmtp::StoredMessage 
{
public:
	G_EXCEPTION( InvalidFormat , "invalid format field in envelope" ) ;
	G_EXCEPTION( NoEnd , "invalid envelope file: misplaced end marker" ) ;
	G_EXCEPTION( InvalidTo , "invalid 'to' line in envelope file" ) ;
	G_EXCEPTION( NoRecipients , "no remote recipients" ) ;
	G_EXCEPTION( OpenError , "cannot open the envelope" ) ;
	G_EXCEPTION( StreamError , "envelope reading/parsing error" ) ;
	G_EXCEPTION( InvalidFilename , "invalid filename" ) ;

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

	virtual std::string name() const ;
		///< Final override from GSmtp::StoredMessage.

	virtual std::string location() const ;
		///< Final override from GSmtp::StoredMessage.

	virtual bool eightBit() const ;
		///< Final override from GSmtp::StoredMessage.

	virtual const std::string & from() const ;
		///< Final override from GSmtp::StoredMessage.

	virtual const G::Strings & to() const ;
		///< Final override from GSmtp::StoredMessage.

	virtual std::string authentication() const ;
		///< Final override from GSmtp::StoredMessage.

	virtual void destroy() ;
		///< Final override from GSmtp::StoredMessage.

	virtual void fail( const std::string & reason , int reason_code ) ;
		///< Final override from GSmtp::StoredMessage.

	virtual void unfail() ;
		///< Final override from GSmtp::StoredMessage.

	virtual std::auto_ptr<std::istream> extractContentStream() ;
		///< Final override from GSmtp::StoredMessage.

	virtual size_t remoteRecipientCount() const ;
		///< Final override from GSmtp::StoredMessage.

	virtual size_t errorCount() const ;
		///< Final override from GSmtp::StoredMessage.

	virtual void sync() ;
		///< Final override from GSmtp::StoredMessage.

private:
	StoredFile( const StoredFile & ) ; // not implemented
	void operator=( const StoredFile & ) ; // not implemented
	static const std::string & crlf() ;
	std::string getline( std::istream & stream ) const ;
	std::string value( const std::string & s , const std::string & k = std::string() ) const ;
	G::Path contentPath() const ;
	void readFormat( std::istream & stream ) ;
	void readFlag( std::istream & stream ) ;
	void readFrom( std::istream & stream ) ;
	void readToList( std::istream & stream ) ;
	void readEnd( std::istream & stream ) ;
	void readReasons( std::istream & stream ) ;
	void readAuthentication( std::istream & stream ) ;
	void readClientSocketAddress( std::istream & stream ) ;
	void readClientSocketName( std::istream & stream ) ;
	void readClientCertificate( std::istream & stream ) ;
	void readEnvelopeCore( bool ) ;
	static void addReason( const G::Path & path , const std::string & , int ) ;
	static G::Path badPath( G::Path ) ;
	void unlock() ;

private:
	FileStore & m_store ;
	G::Strings m_to_local ;
	G::Strings m_to_remote ;
	std::string m_from ;
	G::Path m_envelope_path ;
	G::Path m_old_envelope_path ;
	std::string m_name ;
	std::auto_ptr<std::istream> m_content ;
	bool m_eight_bit ;
	std::string m_authentication ;
	std::string m_format ;
	std::string m_client_socket_address ;
	std::string m_client_socket_name ;
	std::string m_client_certificate ;
	size_t m_errors ;
	bool m_locked ;
} ;

#endif

