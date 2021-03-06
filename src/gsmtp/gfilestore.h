//
// Copyright (C) 2001-2020 Graeme Walker <graeme_walker@users.sourceforge.net>
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

#ifndef G_SMTP_FILE_STORE__H
#define G_SMTP_FILE_STORE__H

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

/// \class GSmtp::FileStore
/// A concrete implementation of the MessageStore interface dealing
/// in paired flat files.
///
/// The implementation puts separate envelope and content files
/// in the spool directory. The content file is written first.
/// The presence of a matching envelope file is used to indicate
/// that the content file is valid and that it has been commited
/// to the care of the SMTP system for delivery.
///
/// Passes out unique sequence numbers, filesystem paths and
/// i/o streams to NewMessageImp.
///
class GSmtp::FileStore : public MessageStore
{
public:
	G_EXCEPTION( InvalidDirectory , "invalid spool directory" ) ;
	G_EXCEPTION( GetError , "error reading specific message" ) ;

	FileStore( const G::Path & dir , bool optimise_empty_test ,
		unsigned long max_size , bool test_for_eight_bit ) ;
			///< Constructor. Throws an exception if the storage directory
			///< is invalid.
			///<
			///< If the optimise flag is set then the implementation of
			///< empty() will be efficient for an empty filestore
			///< (ignoring failed and local-delivery messages). This
			///< might be useful for applications in which the main
			///< event loop is used to check for pending jobs. The
			///< disadvantage is that this process will not be
			///< sensititive to messages deposited into its spool
			///< directory by other processes.

	unsigned long newSeq() ;
		///< Hands out a new non-zero sequence number.

	std::unique_ptr<std::ofstream> stream( const G::Path & path ) ;
		///< Returns a stream to the given content.

	G::Path contentPath( unsigned long seq ) const ;
		///< Returns the path for a content file.

	G::Path envelopePath( unsigned long seq ) const ;
		///< Returns the path for an envelope file.

	G::Path envelopeWorkingPath( unsigned long seq ) const ;
		///< Returns the path for an envelope file
		///< which is in the process of being written.

	bool empty() const override ;
		///< Override from GSmtp::MessageStore.

	std::unique_ptr<StoredMessage> get( unsigned long id ) override ;
		///< Override from GSmtp::MessageStore.

	std::shared_ptr<MessageStore::Iterator> iterator( bool lock ) override ;
		///< Override from GSmtp::MessageStore.

	std::shared_ptr<MessageStore::Iterator> failures() override ;
		///< Override from GSmtp::MessageStore.

	std::unique_ptr<NewMessage> newMessage( const std::string & from ,
		const std::string & from_auth_in , const std::string & from_auth_out ) override ;
			///< Override from GSmtp::MessageStore.

	static std::string x() ;
		///< Returns the prefix for envelope header lines.

	static std::string format( int generation = 0 ) ;
		///< Returns an identifier for the storage format implemented
		///< by this class, or some older generation of it (eg. -1).

	static bool knownFormat( const std::string & format ) ;
		///< Returns true if the storage format string is
		///< recognised and supported for reading.

	void updated() override ;
		///< Override from GSmtp::MessageStore.

	G::Slot::Signal<> & messageStoreUpdateSignal() override ;
		///< Override from GSmtp::MessageStore.

	G::Slot::Signal<> & messageStoreRescanSignal() override ;
		///< Override from GSmtp::MessageStore.

private: // overrides
	void rescan() override ; // Override from GSmtp::MessageStore.
	void unfailAll() override ; // Override from GSmtp::MessageStore.

public:
	~FileStore() override = default ;
	FileStore( const FileStore & ) = delete ;
	FileStore( FileStore && ) = delete ;
	void operator=( const FileStore & ) = delete ;
	void operator=( FileStore && ) = delete ;

private:
	static void checkPath( const G::Path & dir ) ;
	G::Path fullPath( const std::string & filename ) const ;
	std::string filePrefix( unsigned long seq ) const ;
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
	bool m_optimise ;
	mutable bool m_empty ;
	unsigned long m_max_size ;
	bool m_test_for_eight_bit ;
	unsigned long m_pid_modifier ;
	G::Slot::Signal<> m_update_signal ;
	G::Slot::Signal<> m_rescan_signal ;
} ;

/// \class GSmtp::FileReader
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

/// \class GSmtp::DirectoryReader
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

/// \class GSmtp::FileWriter
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

#endif
