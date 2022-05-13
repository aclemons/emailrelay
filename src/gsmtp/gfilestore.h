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
/// \file gfilestore.h
///

#ifndef G_SMTP_FILE_STORE_H
#define G_SMTP_FILE_STORE_H

#include "gdef.h"
#include "gmessagestore.h"
#include "gdatetime.h"
#include "gexception.h"
#include "gprocess.h"
#include "gslot.h"
#include "groot.h"
#include "gpath.h"
#include <fstream>
#include <memory>
#include <string>

namespace GSmtp
{
	class FileStore ;
	class FileReader ;
	class FileWriter ;
	class DirectoryReader ;
}

//| \class GSmtp::FileStore
/// A concrete implementation of the MessageStore interface dealing
/// in paired flat files.
///
/// The implementation puts separate envelope and content files
/// in the spool directory. The content file is written first.
/// The presence of a matching envelope file is used to indicate
/// that the content file is valid and that it has been commited
/// to the care of the SMTP system for delivery.
///
class GSmtp::FileStore : public MessageStore
{
public:
	G_EXCEPTION( InvalidDirectory , tx("invalid spool directory") ) ;
	G_EXCEPTION( GetError , tx("error reading specific message") ) ;
	enum class State // see GSmtp::FileStore::envelopePath()
	{
		Normal ,
		New ,
		Locked
	} ;
	struct Config /// Configuration structure for GSmtp::FileStore.
	{
		std::size_t max_size ; // SIZE in EHLO response
		Config & set_max_size( std::size_t ) noexcept ;
	} ;

	FileStore( const G::Path & dir , const Config & config ) ;
		///< Constructor. Throws an exception if the storage directory
		///< is invalid.

	MessageId newId() ;
		///< Hands out a new unique message id.

	std::unique_ptr<std::ofstream> stream( const G::Path & path ) ;
		///< Returns a stream to the given content.

	G::Path contentPath( const MessageId & id ) const ;
		///< Returns the path for a content file.

	G::Path envelopePath( const MessageId & id , State = State::Normal ) const ;
		///< Returns the path for an envelope file.

	static std::string x() ;
		///< Returns the prefix for envelope header lines.

	static std::string format( int generation = 0 ) ;
		///< Returns an identifier for the storage format implemented
		///< by this class, or some older generation of it (eg. -1).

	static bool knownFormat( const std::string & format ) ;
		///< Returns true if the storage format string is
		///< recognised and supported for reading.

private: // overrides
	bool empty() const override ;
	std::string location( const MessageId & ) const override ;
	std::unique_ptr<StoredMessage> get( const MessageId & ) override ;
	std::shared_ptr<MessageStore::Iterator> iterator( bool lock ) override ;
	std::shared_ptr<MessageStore::Iterator> failures() override ;
	std::unique_ptr<NewMessage> newMessage( const std::string & , const MessageStore::SmtpInfo & , const std::string & ) override ;
	void updated() override ;
	G::Slot::Signal<> & messageStoreUpdateSignal() override ;
	G::Slot::Signal<> & messageStoreRescanSignal() override ;
	void rescan() override ;
	void unfailAll() override ;

public:
	~FileStore() override = default ;
	FileStore( const FileStore & ) = delete ;
	FileStore( FileStore && ) = delete ;
	void operator=( const FileStore & ) = delete ;
	void operator=( FileStore && ) = delete ;

private:
	static void checkPath( const G::Path & dir ) ;
	G::Path fullPath( const std::string & filename ) const ;
	std::string getline( std::istream & ) const ;
	std::string value( const std::string & ) const ;
	std::shared_ptr<MessageStore::Iterator> iteratorImp( bool ) ;
	void unfailAllImp() ;
	static const std::string & crlf() ;
	bool emptyCore() const ;
	void clearAll() ;

private:
	unsigned long m_seq ;
	G::Path m_dir ;
	const Config m_config ;
	G::Slot::Signal<> m_update_signal ;
	G::Slot::Signal<> m_rescan_signal ;
} ;

//| \class GSmtp::FileReader
/// Used by GSmtp::FileStore, GSmtp::NewFile and GSmtp::StoredFile to
/// claim read permissions for reading a file.
/// \see G::Root
///
class GSmtp::FileReader : private G::Root
{
public:
	FileReader() ;
		///< Default constructor. Switches identity for
		///< reading a file.

	~FileReader() ;
		///< Destructor. Switches identity back.

public:
	FileReader( const FileReader & ) = delete ;
	FileReader( FileReader && ) = delete ;
	void operator=( const FileReader & ) = delete ;
	void operator=( FileReader && ) = delete ;
} ;

//| \class GSmtp::DirectoryReader
/// Used by GSmtp::FileStore, GSmtp::NewFile and GSmtp::StoredFile to
/// claim read permissions for reading a directory.
/// \see G::Root
///
class GSmtp::DirectoryReader : private G::Root
{
public:
	DirectoryReader() ;
		///< Default constructor. Switches identity for
		///< reading a directory.

	~DirectoryReader() ;
		///< Destructor. Switches identity back.

public:
	DirectoryReader( const DirectoryReader & ) = delete ;
	DirectoryReader( DirectoryReader && ) = delete ;
	void operator=( const DirectoryReader & ) = delete ;
	void operator=( DirectoryReader && ) = delete ;
} ;

//| \class GSmtp::FileWriter
/// Used by GSmtp::FileStore, GSmtp::NewFile and GSmtp::StoredFile to
/// claim write permissions.
/// \see G::Root
///
class GSmtp::FileWriter : private G::Root , private G::Process::Umask
{
public:
	FileWriter() ;
		///< Default constructor. Switches identity for
		///< writing a file.

	~FileWriter() ;
		///< Destructor. Switches identity back.

public:
	FileWriter( const FileWriter & ) = delete ;
	FileWriter( FileWriter && ) = delete ;
	void operator=( const FileWriter & ) = delete ;
	void operator=( FileWriter && ) = delete ;
} ;

inline GSmtp::FileStore::Config & GSmtp::FileStore::Config::set_max_size( std::size_t n ) noexcept { max_size = n ; return *this ; }

#endif
