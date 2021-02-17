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
/// \file gnewfile.h
///

#ifndef G_SMTP_NEW_FILE_H
#define G_SMTP_NEW_FILE_H

#include "gdef.h"
#include "gfilestore.h"
#include "genvelope.h"
#include "gstrings.h"
#include "gnewmessage.h"
#include "gexception.h"
#include <fstream>

namespace GSmtp
{
	class NewFile ;
}

//| \class GSmtp::NewFile
/// A concrete derived class implementing the GSmtp::NewMessage
/// interface. Writes itself to the i/o streams supplied by
/// GSmtp::FileStore.
///
class GSmtp::NewFile : public NewMessage
{
public:
	G_EXCEPTION( InvalidPath , "invalid path: must be absolute" ) ;
	G_EXCEPTION( FileError , "message store error" ) ;

	NewFile( FileStore & store , const std::string & from , const std::string & from_auth_in ,
		const std::string & from_auth_out , std::size_t max_size , bool test_for_eight_bit ) ;
			///< Constructor. The FileStore reference is kept.

	~NewFile() override ;
		///< Destructor. If the new message has not been
		///< commit()ed then the files are deleted.

	G::Path contentPath() const ;
		///< Returns the path of the content file.

public:
	NewFile( const NewFile & ) = delete ;
	NewFile( NewFile && ) = delete ;
	void operator=( const NewFile & ) = delete ;
	void operator=( NewFile && ) = delete ;

private: // overrides
	void commit( bool strict ) override ; // Override from GSmtp::NewMessage.
	unsigned long id() const override ; // Override from GSmtp::NewMessage.
	void addTo( const std::string & to , bool local ) override ; // Override from GSmtp::NewMessage.
	bool addText( const char * , std::size_t ) override ; // Override from GSmtp::NewMessage.
	std::string prepare( const std::string & auth_id , const std::string & peer_socket_address ,
		const std::string & peer_certificate ) override ; // Override from GSmtp::NewMessage.

private:
	void cleanup() ;
	void flushContent() ;
	void discardContent() ;
	bool commitEnvelope() ;
	void deleteContent() ;
	void deleteEnvelope() ;
	static bool isEightBit( const char * , std::size_t ) ;
	bool saveEnvelope() ;
	void moveToLocal( const G::Path & , const G::Path & , const G::Path & ) ;
	void copyToLocal( const G::Path & , const G::Path & , const G::Path & ) ;

private:
	FileStore & m_store ;
	unsigned long m_seq ;
	std::unique_ptr<std::ofstream> m_content ;
	G::Path m_content_path ;
	G::Path m_envelope_path_0 ;
	G::Path m_envelope_path_1 ;
	bool m_committed ;
	bool m_test_for_eight_bit ;
	bool m_saved ;
	std::size_t m_size ;
	std::size_t m_max_size ;
	Envelope m_env ;
} ;

#endif
