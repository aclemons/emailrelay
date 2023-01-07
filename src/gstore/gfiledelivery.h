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
/// \file gfiledelivery.h
///

#ifndef G_FILE_DELIVERY_H
#define G_FILE_DELIVERY_H

#include "gdef.h"
#include "gmessagedelivery.h"
#include "gfilestore.h"
#include "genvelope.h"
#include "gpath.h"
#include <fstream>

namespace GStore
{
	class FileDelivery ;
}

//| \class GStore::FileDelivery
/// Provides a way to deliver a message to local recipients'
/// mailboxes.
///
class GStore::FileDelivery : public MessageDelivery
{
public:
	FileDelivery( FileStore & , const std::string & domain ) ;
		///< Constructor. Messages for delivery (with extension ".new")
		///< are taken from the file store and delivered to mailbox
		///< sub-directories.
		///<
		///< The recipient addresses are analysed with respect to the
		///< given (non-empty) domain.

	FileDelivery( FileStore & , const std::string & domain , const G::Path & dst ) ;
		///< Constructor. Messages for delivery (with extension ".local")
		///< are taken from the file store and delivered to mailbox
		///< sub-directories of the given base directory.
		///<
		///< Delivery is a no-op if the destination directory is empty.
		///<
		///< The recipient addresses are analysed with respect to the
		///< given (non-empty) domain.

private: // overrides
	void deliver( const MessageId & ) override ; // GStore::MessageDelivery

private:
	bool deliverImp( const G::Path & , const G::Path & , const G::Path & ) ;
	static bool lookup( const std::string & ) ;
	static int mkdir( const G::Path & ) ;
	static bool openIn( std::ifstream & , const G::Path & ) ;
	static bool openOut( std::ofstream & , const G::Path & ) ;
	static bool hardlink( const G::Path & , const G::Path & ) ;
	static void rename( const G::Path & , const G::Path & ) ;
	static std::string normalise( const std::string & ) ;
	static GStore::Envelope readEnvelope( const G::Path & ) ;
	static void writeEnvelope( const GStore::Envelope & , const G::Path & , const GStore::Envelope & ) ;
	static G::StringArray mailboxes( const GStore::Envelope & , const std::string & ) ;
	static std::string mailbox( const std::string & , const std::string & ) ;
	static std::string short_( const G::Path & ) ;

private:
	bool m_active ;
	FileStore & m_store ;
	std::string m_domain ;
	G::Path m_dst ;
	bool m_local_files ;
} ;

#endif
