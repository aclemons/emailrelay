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
/// \file gfilestore.h
///

#ifndef G_SMTP_FILE_STORE_H
#define G_SMTP_FILE_STORE_H

#include "gdef.h"
#include "gsmtp.h"
#include "gmessagestore.h"
#include "gdatetime.h"
#include "gexception.h"
#include "gprocess.h"
#include "gnoncopyable.h"
#include "gslot.h"
#include "groot.h"
#include "gpath.h"
#include "gexecutable.h"
#include <memory>
#include <string>

/// \namespace GSmtp
namespace GSmtp
{
	class FileStore ;
	class FileReader ;
	class FileWriter ;
	class DirectoryReader ;
}

/// \class GSmtp::FileStore
/// A concrete implementation of the MessageStore
/// interface dealing in paired flat files and with an optional
/// external preprocessor program which is used to process files 
/// once they have been stored.
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
class GSmtp::FileStore : public GSmtp::MessageStore 
{
public:
	G_EXCEPTION( InvalidDirectory , "invalid spool directory" ) ;
	G_EXCEPTION( GetError , "error reading specific message" ) ;

	FileStore( const G::Path & dir , bool optimise = false , unsigned long max_size = 0UL ) ;
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

	std::auto_ptr<std::ostream> stream( const G::Path & path ) ;
		///< Returns a stream to the given content.

	G::Path contentPath( unsigned long seq ) const ;
		///< Returns the path for a content file.

	G::Path envelopePath( unsigned long seq ) const ;
		///< Returns the path for an envelope file.

	G::Path envelopeWorkingPath( unsigned long seq ) const ;
		///< Returns the path for an envelope file
		///< which is in the process of being written.

	virtual bool empty() const ;
		///< Final override from GSmtp::MessageStore.

	virtual std::auto_ptr<StoredMessage> get( unsigned long id ) ;
		///< Final override from GSmtp::MessageStore.

	virtual MessageStore::Iterator iterator( bool lock ) ;
		///< Final override from GSmtp::MessageStore.

	virtual MessageStore::Iterator failures() ;
		///< Final override from GSmtp::MessageStore.

	virtual std::auto_ptr<NewMessage> newMessage( const std::string & from ) ;
		///< Final override from GSmtp::MessageStore.

	static std::string x() ;
		///< Returns the prefix for envelope header lines.

	static std::string format() ;
		///< Returns an identifier for the storage format
		///< implemented by this class.

	static bool knownFormat( const std::string & format ) ;
		///< Returns true if the storage format string is
		///< recognised and supported for reading.

	virtual void repoll() ;
		///< Final override from GSmtp::MessageStore.

	virtual void unfailAll() ;
		///< Final override from GSmtp::MessageStore.

	virtual void updated() ;
		///< Final override from GSmtp::MessageStore.

	virtual G::Signal1<bool> & signal() ;
		///< Final override from GSmtp::MessageStore.

private:
	static void checkPath( const G::Path & dir ) ;
	G::Path fullPath( const std::string & filename ) const ;
	std::string filePrefix( unsigned long seq ) const ;
	std::string getline( std::istream & ) const ;
	std::string value( const std::string & ) const ;
	static const std::string & crlf() ;
	bool emptyCore() const ;

private:
	unsigned long m_seq ;
	G::Path m_dir ;
	bool m_optimise ;
	bool m_empty ; // mutable
	bool m_repoll ;
	unsigned long m_max_size ;
	unsigned long m_pid_modifier ;
	G::Signal1<bool> m_signal ;
} ;

/// \class GSmtp::FileReader
/// Used by GSmtp::FileStore, GSmtp::NewFile and
/// GSmtp::StoredFile to claim read permissions for 
/// reading a file.
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
} ;

/// \class GSmtp::DirectoryReader
/// Used by GSmtp::FileStore, GSmtp::NewFile and
/// GSmtp::StoredFile to claim read permissions for 
/// reading a directory.
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
} ;

/// \class GSmtp::FileWriter
/// Used by GSmtp::FileStore, GSmtp::NewFile and
/// GSmtp::StoredFile to claim write permissions.
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
} ;

#endif

