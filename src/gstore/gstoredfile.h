//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "gfbuf.h"
#include "gpath.h"
#include "gstringarray.h"
#include <iostream>
#include <memory>
#include <new>
#include <cstdio>

namespace GStore
{
	class StoredFile ;
	class FileIterator ;
}

//| \class GStore::StoredFile
/// A concete class implementing the GStore::StoredMessage interface
/// for separate envelope and content files in a spool directory.
/// The GStore::MessageStore::Iterator interface is normally used to
/// retrieve StoredFile instances.
///
/// \see GStore::FileStore, GStore::MessageStore::Iterator, GPop::Store
///
class GStore::StoredFile : public StoredMessage
{
public:
	G_EXCEPTION( FormatError , tx("invalid envelope file") )
	G_EXCEPTION( FilenameError , tx("invalid envelope filename") )
	G_EXCEPTION( EditError , tx("cannot update envelope file") )
	G_EXCEPTION( SizeError , tx("cannot get content file size") )
	using State = FileStore::State ;

	StoredFile( FileStore & store , const MessageId & , State = State::Normal ) ;
		///< Constructor.

	~StoredFile() override ;
		///< Destructor. Unlocks the file if it has been lock()ed
		///< but not destroy()ed or fail()ed or noUnlock()ed.

	bool lock() ;
		///< Locks the file by renaming the envelope file.

	bool readEnvelope( std::string & reason ) ;
		///< Reads the envelope. Returns false on error with a
		///< reason; does not throw.

	bool openContent( std::string & reason ) ;
		///< Opens the content file. Returns false on error.

	MessageId id() const override ;
		///< Override from GStore::StoredMessage.

	void noUnlock() ;
		///< Disable unlocking in the destructor.

	void editEnvelope( std::function<void(Envelope&)> , std::istream * headers = nullptr ) ;
		///< Edits the envelope and updates it in the file store.
		///< Optionally adds more trailing headers.

private: // overrides
	void fail( const std::string & reason , int reason_code ) override ; // GStore::StoredMessage
	std::string location() const override ; // GStore::StoredMessage
	MessageStore::BodyType bodyType() const override ; // GStore::StoredMessage
	std::string from() const override ; // GStore::StoredMessage
	std::string to( std::size_t ) const override ; // GStore::StoredMessage
	std::size_t toCount() const override ; // GStore::StoredMessage
	std::string authentication() const override ; // GStore::StoredMessage
	std::string fromAuthIn() const override ; // GStore::StoredMessage
	std::string fromAuthOut() const override ; // GStore::StoredMessage
	std::string forwardTo() const override ; // GStore::StoredMessage
	std::string forwardToAddress() const override ; // GStore::StoredMessage
	std::string clientAccountSelector() const override ; // GStore::StoredMessage
	bool utf8Mailboxes() const override ; // GStore::StoredMessage
	void close() override ; // GStore::StoredMessage
	std::string reopen() override ; // GStore::StoredMessage
	void destroy() override ; // GStore::StoredMessage
	std::size_t contentSize() const override ; // GStore::StoredMessage
	std::istream & contentStream() override ; // GStore::StoredMessage
	void editRecipients( const G::StringArray & ) override ; // GStore::StoredMessage

public:
	StoredFile( const StoredFile & ) = delete ;
	StoredFile( StoredFile && ) = delete ;
	StoredFile & operator=( const StoredFile & ) = delete ;
	StoredFile & operator=( StoredFile && ) = delete ;

private:
	using FileOp = FileStore::FileOp ;
	using StreamBuf = G::fbuf<int,BUFSIZ> ;
	struct Stream : StreamBuf , std::istream
	{
		Stream() ;
		explicit Stream( const G::Path & ) ;
		void open( const G::Path & ) ;
		std::streamoff size() const ;
	} ;

private:
	G::Path cpath() const ;
	G::Path epath( State ) const ;
	void replaceEnvelope( const G::Path & , const G::Path & ) ;
	const std::string & eol() const ;
	void addReason( const G::Path & path , const std::string & , int ) const ;
	static std::size_t writeEnvelopeImp( const Envelope & , const G::Path & , std::ofstream & ) ;

private:
	FileStore & m_store ;
	std::unique_ptr<Stream> m_content ;
	MessageId m_id ;
	Envelope m_env ;
	State m_state ;
	bool m_unlock {false} ;
} ;

#endif
