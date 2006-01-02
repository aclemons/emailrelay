//
// Copyright (C) 2001-2006 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later
// version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
// ===
//
// gnewfile.h
//

#ifndef G_SMTP_NEW_FILE_H
#define G_SMTP_NEW_FILE_H

#include "gdef.h"
#include "gsmtp.h"
#include "gfilestore.h"
#include "gstrings.h"
#include "gnewmessage.h"
#include "gexception.h"
#include <iostream>

namespace GSmtp
{
	class NewFile ;
}

// Class: GSmtp::NewFile
// Description: A concrete derived class implementing the 
// NewMessage interface. Writes itself to the i/o streams 
// supplied by MessageStoreImp.
//
class GSmtp::NewFile : public GSmtp::NewMessage 
{
public:
	G_EXCEPTION( InvalidPath , "invalid path -- must be absolute" ) ;

	NewFile( const std::string & from , FileStore & store ) ;
		// Constructor.

	virtual ~NewFile() ;
		// Destructor. If the new message has not been
		// commit()ed then the files are deleted.

	virtual void addTo( const std::string & to , bool local ) ;
		// Adds a 'to' address.

	virtual void addText( const std::string & line ) ;
		// Adds a line of content.

	virtual std::string prepare( const std::string & auth_id , const std::string & client_ip ) ;
		// Prepares to store the message in the message store.
		//
		// The implementation flushes and closes the
		// content stream, creates a new envelope
		// file (".new"), and does any local 'delivery'
		// by creating ".local" copies. The path
		// to the ".new" envelope file is returned.

	virtual void commit() ;
		// Commits the message to the message store.
		//
		// The implementation renames the ".new"
		// envelope file, removing the extension.

	virtual unsigned long id() const ;
		// Returns the message's unique non-zero identifier.

	G::Path contentPath() const ;
		// Returns the path of the content file.

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

private:
	void cleanup() ;
	void flushContent() ;
	void discardContent() ;
	bool commitEnvelope() ;
	void rollback() ;
	void deleteContent() ;
	void deleteEnvelope() ;
	static bool isEightBit( const std::string & line ) ;
	const std::string & crlf() const ;
	bool saveEnvelope( const std::string & auth_id , const std::string & client_ip ) const ;
	void writeEnvelope( std::ostream & , const std::string & where , 
		const std::string & auth_id , const std::string & client_ip ) const ;
	void deliver( const G::Strings & , const G::Path & , const G::Path & , const G::Path & ) ;
} ;

#endif
