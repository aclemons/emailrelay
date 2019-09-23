//
// Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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

#ifndef G_SMTP_NEW_FILE__H
#define G_SMTP_NEW_FILE__H

#include "gdef.h"
#include "gfilestore.h"
#include "gstrings.h"
#include "gnewmessage.h"
#include "gexception.h"
#include <fstream>

namespace GSmtp
{
	class NewFile ;
}

/// \class GSmtp::NewFile
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
		const std::string & from_auth_out , size_t max_size , bool test_for_eight_bit ) ;
			///< Constructor. The FileStore reference is kept.

	virtual ~NewFile() ;
		///< Destructor. If the new message has not been
		///< commit()ed then the files are deleted.

	G::Path contentPath() const ;
		///< Returns the path of the content file.

private: // overrides
	virtual void commit( bool strict ) override ; // Override from GSmtp::NewMessage.
	virtual unsigned long id() const override ; // Override from GSmtp::NewMessage.
	virtual void addTo( const std::string & to , bool local ) override ; // Override from GSmtp::NewMessage.
	virtual bool addText( const char * , size_t ) override ; // Override from GSmtp::NewMessage.
	virtual std::string prepare( const std::string & auth_id , const std::string & peer_socket_address ,
		const std::string & peer_certificate ) override ; // Override from GSmtp::NewMessage.

private:
	void cleanup() ;
	void flushContent() ;
	void discardContent() ;
	bool commitEnvelope() ;
	void deleteContent() ;
	void deleteEnvelope() ;
	static bool isEightBit( const char * , size_t ) ;
	static const char * crlf() ;
	static std::string xnormalise( const std::string & ) ;
	bool saveEnvelope( const std::string & auth_id , const std::string & peer_socket_address ,
		const std::string & peer_certificate ) const ;
	void writeEnvelope( std::ofstream & , const std::string & where ,
		const std::string & auth_id , const std::string & peer_socket_address ,
		const std::string & peer_certificate ) const ;
	void moveToLocal( const G::Path & , const G::Path & , const G::Path & ) ;
	void copyToLocal( const G::Path & , const G::Path & , const G::Path & ) ;

private:
	FileStore & m_store ;
	unsigned long m_seq ;
	std::string m_from ;
	std::string m_from_auth_in ;
	std::string m_from_auth_out ;
	G::StringArray m_to_local ;
	G::StringArray m_to_remote ;
	unique_ptr<std::ofstream> m_content ;
	G::Path m_content_path ;
	G::Path m_envelope_path_0 ;
	G::Path m_envelope_path_1 ;
	bool m_committed ;
	bool m_test_for_eight_bit ;
	int m_eight_bit ;
	bool m_saved ;
	size_t m_size ;
	size_t m_max_size ;
} ;

#endif
