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
#include "gstringarray.h"
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
	G_EXCEPTION( FormatError , tx("invalid envelope file") ) ;
	G_EXCEPTION( FilenameError , tx("invalid envelope filename") ) ;
	G_EXCEPTION( ReadError , tx("cannot read envelope file") ) ;
	G_EXCEPTION( EditError , tx("cannot update envelope file") ) ;
	G_EXCEPTION( SizeError , tx("cannot get content file size") ) ;

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

	MessageId id() const override ;
		///< Override from GSmtp::StoredMessage.

private: // overrides
	void edit( const G::StringArray & ) override ; // GSmtp::StoredMessage
	void fail( const std::string & reason , int reason_code ) override ; // GSmtp::StoredMessage
	std::string location() const override ; // GSmtp::StoredMessage
	MessageStore::BodyType bodyType() const override ; // GSmtp::StoredMessage
	std::string from() const override ; // GSmtp::StoredMessage
	std::string to( std::size_t ) const override ; // GSmtp::StoredMessage
	std::size_t toCount() const override ; // GSmtp::StoredMessage
	std::string authentication() const override ; // GSmtp::StoredMessage
	std::string fromAuthIn() const override ; // GSmtp::StoredMessage
	std::string fromAuthOut() const override ; // GSmtp::StoredMessage
	bool utf8Mailboxes() const override ; // GSmtp::StoredMessage
	void close() override ; // GSmtp::StoredMessage
	std::string reopen() override ; // GSmtp::StoredMessage
	void destroy() override ; // GSmtp::StoredMessage
	void unfail() override ; // GSmtp::StoredMessage
	std::size_t contentSize() const override ; // GSmtp::StoredMessage
	std::istream & contentStream() override ; // GSmtp::StoredMessage

public:
	StoredFile( const StoredFile & ) = delete ;
	StoredFile( StoredFile && ) = delete ;
	void operator=( const StoredFile & ) = delete ;
	void operator=( StoredFile && ) = delete ;

private:
	enum class State { Normal , Locked , Bad } ;
	G::Path cpath() const ;
	G::Path epath( State ) const ;
	void readEnvelopeCore( bool check_recipients ) ;
	const std::string & eol() const ;
	void addReason( const G::Path & path , const std::string & , int ) const ;

private:
	FileStore & m_store ;
	std::unique_ptr<std::istream> m_content ;
	MessageId m_id ;
	Envelope m_env ;
	State m_state ;
} ;

#endif

