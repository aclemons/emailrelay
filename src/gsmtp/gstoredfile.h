//
// Copyright (C) 2001-2004 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later
// version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
// ===
//
// gstoredfile.h
//

#ifndef G_SMTP_STORED_FILE_H
#define G_SMTP_STORED_FILE_H

#include "gdef.h"
#include "gsmtp.h"
#include "gmessagestore.h"
#include "gexe.h"
#include "gprocessor.h"
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

// Class: GSmtp::StoredFile
// Description: A concete derived class implementing the
// StoredMessage interface.
//
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

	StoredFile( FileStore & store , Processor & store_preprocessor , const G::Path & envelope_path ) ;
		// Constructor.

	virtual ~StoredFile() ;
		// Destructor. Unlocks the file if it has been lock()ed
		// but not destroy()ed or fail()ed.

	bool lock() ;
		// Locks the file by renaming the envelope file.
		// Used by FileStore and FileIterator.

	bool readEnvelope( std::string & reason , bool check_for_no_remote_recipients ) ;
		// Reads the envelope. Returns false on error.
		// Used by FileStore and FileIterator.

	bool openContent( std::string & reason ) ;
		// Opens the content file. Returns false on error.
		// Used by FileStore and FileIterator.

	virtual std::string name() const ;
		// From StoredMessage.

	virtual bool eightBit() const ;
		// From StoredMessage.

	virtual const std::string & from() const ;
		// From StoredMessage.

	virtual const G::Strings & to() const ;
		// From StoredMessage.

	virtual std::string authentication() const ;
		// From StoredMessage.

	virtual bool preprocess() ;
		// From StoredMessage.

	virtual void destroy() ;
		// From StoredMessage.

	virtual void fail( const std::string & reason ) ;
		// From StoredMessage.

	virtual std::auto_ptr<std::istream> extractContentStream() ;
		// From StoredMessage.

	virtual size_t remoteRecipientCount() const ;
		// From StoredMessage.

	virtual size_t errorCount() const ;
		// From StoredMessage.

private:
	StoredFile( const StoredFile & ) ;
	void operator=( const StoredFile & ) ;
	std::string crlf() const ;
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
	void readClientIp( std::istream & stream ) ;
	void readEnvelopeCore( bool ) ;
	void unlock() ;

private:
	FileStore & m_store ;
	Processor & m_store_preprocessor ;
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
	std::string m_client_ip ;
	size_t m_errors ;
	bool m_locked ;
} ;

#endif

