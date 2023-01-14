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
#include "gexception.h"
#include "gpath.h"
#include <fstream>
#include <utility>

namespace GStore
{
	class FileDelivery ;
}

//| \class GStore::FileDelivery
/// An implementation of the MessageDelivery interface that delivers
/// a message to its local recipients' mailboxes.
///
class GStore::FileDelivery : public MessageDelivery
{
public:
	G_EXCEPTION( EnvelopeWriteError , tx("delivery: cannot write envelope file") ) ;
	G_EXCEPTION( ContentWriteError , tx("delivery: cannot write content file") ) ;
	G_EXCEPTION( MkdirError , tx("delivery: cannot create delivery directory") ) ;
	G_EXCEPTION( MaildirCopyError , tx("delivery: cannot write maildir tmp file") ) ;
	G_EXCEPTION( MaildirMoveError , tx("delivery: cannot move maildir file") ) ;

	FileDelivery( FileStore & , const std::string & domain , std::pair<int,int> uid_range = {0,-1} ) ;
		///< Constructor. Messages for delivery with extension ".new"
		///< are taken from the file store and delivered to mailbox
		///< sub-directories.
		///<
		///< The recipient addresses are analysed with respect to
		///< the given (non-empty) domain and uid range; for
		///< recipients that do not match delivery is to "postmaster".

	FileDelivery( FileStore & , const std::string & domain , const G::Path & base_dir ) ;
		///< Constructor. Messages for delivery with extension ".local"
		///< are taken from the file store and delivered to mailbox
		///< sub-directories of the given base directory.
		///<
		///< Delivery is a no-op if the 'base_dir' path is empty or
		///< if the ".local" envelope file does not exist.
		///<
		///< The recipient addresses are analysed with respect to the
		///< given (non-empty) domain.

	static void deliverTo( FileStore & , const G::Path & mbox_dir ,
		const G::Path & envelope_path , const G::Path & content_path ,
		bool hardlink = false ) ;
			///< Low-level function to copy a message into a mailbox.
			///<
			///< Does "maildir" delivery if the mailbox directory contains
			///< tmp/new/cur sub-directories.
			///<
			///< The content file is optionally hard-linked.

private: // overrides
	void deliver( const MessageId & ) override ; // GStore::MessageDelivery

private:
	using FileOp = FileStore::FileOp ;
	bool deliverToMailboxes( const G::Path & , const G::Path & , const G::Path & , std::pair<int,int> ) ;
	static bool lookup( const std::string & , std::pair<int,int> ) ;
	static std::string normalise( const std::string & ) ;
	static G::StringArray mailboxes( const GStore::Envelope & , const std::string & , std::pair<int,int> ) ;
	static std::string mailbox( const std::string & , std::pair<int,int> , const std::string & ) ;
	static std::string short_( const G::Path & ) ;

private:
	bool m_active ;
	FileStore & m_store ;
	std::string m_domain ;
	std::pair<int,int> m_uid_range ;
	G::Path m_base_dir ;
	bool m_local_files ;
} ;

#endif
