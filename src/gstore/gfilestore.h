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
/// \file gfilestore.h
///

#ifndef G_SMTP_FILE_STORE_H
#define G_SMTP_FILE_STORE_H

#include "gdef.h"
#include "gmessagestore.h"
#include "genvelope.h"
#include "gdatetime.h"
#include "gexception.h"
#include "gprocess.h"
#include "gslot.h"
#include "groot.h"
#include "gpath.h"
#include <fstream>
#include <memory>
#include <string>

namespace GStore
{
	class FileStore ;
	class FileReader ;
	class FileWriter ;
	class DirectoryReader ;
}

//| \class GStore::FileStore
/// A concrete implementation of the MessageStore interface dealing
/// in paired flat files.
///
/// The implementation puts separate envelope and content files
/// in the spool directory. The content file is written first.
/// The presence of a matching envelope file is used to indicate
/// that the content file is valid and that it has been commited
/// to the care of the SMTP system for delivery.
///
class GStore::FileStore : public MessageStore
{
public:
	G_EXCEPTION( InvalidDirectory , tx("invalid spool directory") )
	G_EXCEPTION( EnvelopeReadError , tx("cannot read envelope file") )
	G_EXCEPTION( GetError , tx("error getting message") )
	enum class State // see GStore::FileStore::envelopePath()
	{
		New ,
		Normal ,
		Locked ,
		Bad
	} ;
	struct Config /// Configuration structure for GStore::FileStore.
	{
		std::size_t max_size {0U} ; // zero for unlimited -- passed to GStore::NewFile::ctor
		unsigned long seq {0UL} ; // sequence number start
		Config & set_max_size( std::size_t ) noexcept ;
		Config & set_seq( unsigned long ) noexcept ;
	} ;
	struct FileOp /// Low-level file-system operations for GStore::FileStore.
	{
		FileOp() = delete ;
		static int & errno_() noexcept ;
		static bool rename( const G::Path & , const G::Path & ) ;
		static bool renameOnto( const G::Path & , const G::Path & ) ;
		static bool remove( const G::Path & ) noexcept ;
		static bool exists( const G::Path & ) ;
		static int fdopen( const G::Path & ) ;
		static bool hardlink( const G::Path & , const G::Path & ) ;
		static bool copy( const G::Path & , const G::Path & ) ;
		static bool copy( const G::Path & , const G::Path & , bool hardlink ) ;
		static bool mkdir( const G::Path & ) ;
		static bool isdir( const G::Path & , const G::Path & = {} , const G::Path & = {} ) ;
		static std::ifstream & openIn( std::ifstream & , const G::Path & ) ;
		static std::ofstream & openOut( std::ofstream & , const G::Path & ) ;
		static std::ofstream & openAppend( std::ofstream & , const G::Path & ) ;
	} ;

	static G::Path defaultDirectory() ;
		///< Returns a default spool directory, such as "/var/spool/emailrelay".
		///< (Typically with an os-specific implementation.)

	FileStore( const G::Path & spool_dir , const G::Path & delivery_dir , const Config & config ) ;
		///< Constructor. Throws an exception if the spool directory
		///< is invalid.

	G::Path directory() const ;
		///< Returns the spool directory path, as passed in to the
		///< constructor.

	G::Path deliveryDir() const ;
		///< Returns the base directory for local delivery. Returns
		///< directory() by default.

	MessageId newId() ;
		///< Hands out a new unique message id.

	static std::unique_ptr<std::ofstream> stream( const G::Path & path ) ;
		///< Opens an output stream to a message file using the appropriate
		///< effective userid and umask.

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

	static Envelope readEnvelope( const G::Path & , std::ifstream * = nullptr ) ;
		///< Used by FileStore sibling classes to read an envelope file.
		///< Optionally returns the newly-opened stream by reference so
		///< that any trailing headers can be read. Throws on error.

private: // overrides
	bool empty() const override ;
	std::string location( const MessageId & ) const override ;
	std::unique_ptr<StoredMessage> get( const MessageId & ) override ;
	std::unique_ptr<MessageStore::Iterator> iterator( bool lock ) override ;
	std::unique_ptr<NewMessage> newMessage( const std::string & , const MessageStore::SmtpInfo & , const std::string & ) override ;
	void updated() override ;
	G::Slot::Signal<> & messageStoreUpdateSignal() noexcept override ;
	G::Slot::Signal<> & messageStoreRescanSignal() noexcept override ;
	std::vector<MessageId> ids() override ;
	std::vector<MessageId> failures() override ;
	void unfailAll() override ;
	void rescan() override ;

public:
	~FileStore() override = default ;
	FileStore( const FileStore & ) = delete ;
	FileStore( FileStore && ) = delete ;
	FileStore & operator=( const FileStore & ) = delete ;
	FileStore & operator=( FileStore && ) = delete ;

private:
	static void checkPath( const G::Path & dir ) ;
	static void osinit() ;
	G::Path fullPath( const std::string & filename ) const ;
	std::string getline( std::istream & ) const ;
	std::string value( const std::string & ) const ;
	static const std::string & crlf() ;
	bool emptyCore() const ;
	void clearAll() ;
	static MessageId newId( unsigned long ) ;

private:
	unsigned long m_seq ;
	G::Path m_dir ;
	G::Path m_delivery_dir ;
	const Config m_config ;
	G::Slot::Signal<> m_update_signal ;
	G::Slot::Signal<> m_rescan_signal ;
} ;

//| \class GStore::FileReader
/// Used by GStore::FileStore, GStore::NewFile and GStore::StoredFile to
/// claim read permissions for reading a file.
/// \see G::Root
///
class GStore::FileReader : private G::Root
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
	FileReader & operator=( const FileReader & ) = delete ;
	FileReader & operator=( FileReader && ) = delete ;
} ;

//| \class GStore::DirectoryReader
/// Used by GStore::FileStore, GStore::NewFile and GStore::StoredFile to
/// claim read permissions for reading a directory.
/// \see G::Root
///
class GStore::DirectoryReader : private G::Root
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
	DirectoryReader & operator=( const DirectoryReader & ) = delete ;
	DirectoryReader & operator=( DirectoryReader && ) = delete ;
} ;

//| \class GStore::FileWriter
/// Used by GStore::FileStore, GStore::NewFile and GStore::StoredFile to
/// claim write permissions.
/// \see G::Root
///
class GStore::FileWriter : private G::Root , private G::Process::Umask
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
	FileWriter & operator=( const FileWriter & ) = delete ;
	FileWriter & operator=( FileWriter && ) = delete ;
} ;

inline GStore::FileStore::Config & GStore::FileStore::Config::set_max_size( std::size_t n ) noexcept { max_size = n ; return *this ; }
inline GStore::FileStore::Config & GStore::FileStore::Config::set_seq( unsigned long n ) noexcept { seq = n ; return *this ; }

#endif
