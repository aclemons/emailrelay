//
// Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gnewfile.h
///

#ifndef G_SMTP_NEW_FILE_H
#define G_SMTP_NEW_FILE_H

#include "gdef.h"
#include "gfilestore.h"
#include "genvelope.h"
#include "gstringarray.h"
#include "gnewmessage.h"
#include "gexception.h"
#include <memory>
#include <fstream>

namespace GStore
{
	class NewFile ;
}

//| \class GStore::NewFile
/// A concrete class implementing the GStore::NewMessage interface using
/// files.
///
/// The prepare() override creates one ".envelope.new" file and one
/// ".content" file.
///
/// The commit() override renames the envelope file to remove the ".new"
/// filename extension. This makes it visible to FileStore::iterator().
///
class GStore::NewFile : public NewMessage
{
public:
	G_EXCEPTION( FileError , tx("message store error") ) ;

	NewFile( FileStore & store , const std::string & from , const MessageStore::SmtpInfo & ,
		const std::string & from_auth_out , std::size_t max_size ) ;
			///< Constructor. The max-size is the size limit, as also
			///< reported by the EHLO response, and is not the size
			///< estimate from MAIL-FROM.

	~NewFile() override ;
		///< Destructor. If the new message has not been
		///< commit()ed then the files are deleted.

	G::Path contentPath() const ;
		///< Returns the path of the content file.

public:
	NewFile( const NewFile & ) = delete ;
	NewFile( NewFile && ) = delete ;
	NewFile & operator=( const NewFile & ) = delete ;
	NewFile & operator=( NewFile && ) = delete ;

private: // overrides
	void commit( bool strict ) override ; // GStore::NewMessage
	MessageId id() const override ; // GStore::NewMessage
	std::string location() const override ; // GStore::NewMessage
	void addTo( const std::string & to , bool local , bool utf8address ) override ; // GStore::NewMessage
	NewMessage::Status addContent( const char * , std::size_t ) override ; // GStore::NewMessage
	std::size_t contentSize() const override ; // GStore::NewMessage
	void prepare( const std::string & auth_id , const std::string & peer_socket_address ,
		const std::string & peer_certificate ) override ; // GStore::NewMessage

private:
	using FileOp = FileStore::FileOp ;
	enum class State { New , Normal } ;
	G::Path cpath() const ;
	G::Path epath( State ) const ;
	void cleanup() ;
	void saveEnvelope( Envelope & , const G::Path & ) ;

private:
	FileStore & m_store ;
	MessageId m_id ;
	std::unique_ptr<std::ofstream> m_content ;
	bool m_committed ;
	bool m_saved ;
	std::size_t m_size ;
	std::size_t m_max_size ;
	Envelope m_env ;
} ;

#endif
