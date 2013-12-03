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
/// \file gnewfile.h
///

#ifndef G_SMTP_NEW_FILE_H
#define G_SMTP_NEW_FILE_H

#include "gdef.h"
#include "gsmtp.h"
#include "gfilestore.h"
#include "gstrings.h"
#include "gnewmessage.h"
#include "gexception.h"
#include <iostream>

/// \namespace GSmtp
namespace GSmtp
{
	class NewFile ;
}

/// \class GSmtp::NewFile
/// A concrete derived class implementing the 
/// NewMessage interface. Writes itself to the i/o streams 
/// supplied by MessageStoreImp.
///
class GSmtp::NewFile : public GSmtp::NewMessage 
{
public:
	G_EXCEPTION( InvalidPath , "invalid path -- must be absolute" ) ;

	NewFile( const std::string & from , FileStore & store , unsigned long max_size = 0UL ) ;
		///< Constructor. The FileStore reference is kept.

	virtual ~NewFile() ;
		///< Destructor. If the new message has not been
		///< commit()ed then the files are deleted.

	virtual void addTo( const std::string & to , bool local ) ;
		///< Final override from GSmtp::NewMessage.

	virtual bool addText( const std::string & line ) ;
		///< Final override from GSmtp::NewMessage.

	virtual std::string prepare( const std::string & auth_id , const std::string & peer_socket_address ,
		const std::string & peer_socket_name , const std::string & peer_certificate ) ;
			///< Final override from GSmtp::NewMessage.
			///<
			///< The implementation flushes and closes the
			///< content stream, creates a new envelope
			///< file (".new"), and does any local 'delivery'
			///< by creating ".local" copies. The path
			///< to the content file is returned.

	virtual void commit() ;
		///< Final override from GSmtp::NewMessage.
		///<
		///< The implementation renames the ".new"
		///< envelope file, removing the extension.

	virtual unsigned long id() const ;
		///< Final override from GSmtp::NewMessage.

	G::Path contentPath() const ;
		///< Returns the path of the content file.

private:
	FileStore & m_store ;
	unsigned long m_seq ;
	std::string m_from ;
	G::Strings m_to_local ;
	G::Strings m_to_remote ;
	std::auto_ptr<std::ostream> m_content ;
	G::Path m_content_path ;
	G::Path m_envelope_path_0 ;
	G::Path m_envelope_path_1 ;
	bool m_committed ;
	bool m_eight_bit ;
	bool m_saved ;
	unsigned long m_size ;
	unsigned long m_max_size ;

private:
	void cleanup() ;
	void flushContent() ;
	void discardContent() ;
	bool commitEnvelope() ;
	void deleteContent() ;
	void deleteEnvelope() ;
	static bool isEightBit( const std::string & line ) ;
	const std::string & crlf() const ;
	bool saveEnvelope( const std::string & auth_id , const std::string & peer_socket_address ,
		const std::string & peer_socket_name , const std::string & peer_certificate ) const ;
	void writeEnvelope( std::ostream & , const std::string & where , 
		const std::string & auth_id , const std::string & peer_socket_address ,
		const std::string & peer_socket_name , const std::string & peer_certificate ) const ;
	void deliver( const G::Strings & , const G::Path & , const G::Path & , const G::Path & ) ;
} ;

#endif
