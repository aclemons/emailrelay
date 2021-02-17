//
// Copyright (C) 2001-2021 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "gfilestore.h"
#include "gstoredmessage.h"
#include "genvelope.h"
#include "gexception.h"
#include "gpath.h"
#include "gstrings.h"
#include <iostream>
#include <memory>

namespace GSmtp
{
	class StoredFile ;
}

//| \class GSmtp::StoredFile
/// A concete derived class implementing the
/// StoredMessage interface.
///
class GSmtp::StoredFile : public StoredMessage
{
public:
	G_EXCEPTION( FormatError , "invalid envelope file" ) ;
	G_EXCEPTION( FilenameError , "invalid envelope filename" ) ;
	G_EXCEPTION( ReadError , "cannot read envelope file" ) ;
	G_EXCEPTION( EditError , "cannot update envelope file" ) ;

	StoredFile( FileStore & store , const G::Path & envelope_path ) ;
		///< Constructor.

	~StoredFile() override ;
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

	std::string name() const override ;
		///< Override from GSmtp::StoredMessage.

	void edit( const G::StringArray & ) override ;
		///< Override from GSmtp::StoredMessage.

	void fail( const std::string & reason , int reason_code ) override ;
		///< Override from GSmtp::StoredMessage.

private: // overrides
	std::string location() const override ; // Override from GSmtp::StoredMessage.
	int eightBit() const override ; // Override from GSmtp::StoredMessage.
	std::string from() const override ; // Override from GSmtp::StoredMessage.
	std::string to( std::size_t ) const override ; // Override from GSmtp::StoredMessage.
	std::size_t toCount() const override ; // Override from GSmtp::StoredMessage.
	std::string authentication() const override ; // Override from GSmtp::StoredMessage.
	std::string fromAuthIn() const override ; // Override from GSmtp::StoredMessage.
	std::string fromAuthOut() const override ; // Override from GSmtp::StoredMessage.
	void close() override ; // Override from GSmtp::StoredMessage.
	std::string reopen() override ; // Override from GSmtp::StoredMessage.
	void destroy() override ; // Override from GSmtp::StoredMessage.
	void unfail() override ; // Override from GSmtp::StoredMessage.
	std::istream & contentStream() override ; // Override from GSmtp::StoredMessage.

public:
	StoredFile( const StoredFile & ) = delete ;
	StoredFile( StoredFile && ) = delete ;
	void operator=( const StoredFile & ) = delete ;
	void operator=( StoredFile && ) = delete ;

private:
	void readEnvelopeCore( bool check_recipients ) ;
	const std::string & eol() const ;
	G::Path contentPath() const ;
	void addReason( const G::Path & path , const std::string & , int ) const ;
	static G::Path badPath( const G::Path & ) ;

private:
	FileStore & m_store ;
	std::unique_ptr<std::istream> m_content ;
	G::Path m_envelope_path ;
	G::Path m_old_envelope_path ;
	std::string m_name ;
	Envelope m_env ;
	bool m_locked ;
} ;

#endif

