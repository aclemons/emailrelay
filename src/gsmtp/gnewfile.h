//
// Copyright (C) 2001-2004 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "gprocessor.h"
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

	NewFile( const std::string & from , FileStore & store , Processor & store_preprocessor ) ;
		// Constructor. The preprocessor is ignored if
		// its exe() path is empty.

	virtual ~NewFile() ;
		// Destructor.

	virtual void addTo( const std::string & to , bool local ) ;
		// Adds a 'to' address.

	virtual void addText( const std::string & line ) ;
		// Adds a line of content.

	virtual bool store( const std::string & auth_id , const std::string & client_ip ) ;
		// Stores the message in the message store.
		// Returns true if storage was deliberately
		// cancelled.

	virtual unsigned long id() const ;
		// Returns the message's unique non-zero identifier.

	static void setPreprocessor( const G::Path & exe , const G::Strings & exe_args ) ;
		// Defines a program which is used for pre-processing
		// messages before they are stored.

	G::Path contentPath() const ;
		// Returns the path of the content file.

private:
	FileStore & m_store ;
	Processor & m_store_preprocessor ;
	unsigned long m_seq ;
	std::string m_from ;
	G::Strings m_to_local ;
	G::Strings m_to_remote ;
	std::auto_ptr<std::ostream> m_content ;
	G::Path m_content_path ;
	bool m_eight_bit ;
	bool m_saved ;
	bool m_repoll ;

private:
	bool saveEnvelope( std::ostream & , const std::string & where , 
		const std::string & auth_id , const std::string & client_ip ) const ;
	const std::string & crlf() const ;
	static bool isEightBit( const std::string & line ) ;
	void deliver( const G::Strings & , const G::Path & , const G::Path & , const G::Path & ) ;
	bool preprocess( const G::Path & , bool & , std::string & ) ;
	int preprocessCore( const G::Path & , std::string & ) ;
	std::string parseOutput( std::string ) const ;
	bool commit( const G::Path & , const G::Path & ) ;
	void rollback() ;
	void cleanup() ;
} ;

#endif
