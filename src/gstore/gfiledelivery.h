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
/// An implementation of the MessageDelivery interface that delivers a message
/// to mailboxes corresponding to its local and remote recipient addresses.
///
class GStore::FileDelivery : public MessageDelivery
{
public:
	G_EXCEPTION( EnvelopeWriteError , tx("delivery: cannot write envelope file") ) ;
	G_EXCEPTION( ContentWriteError , tx("delivery: cannot write content file") ) ;
	G_EXCEPTION( MkdirError , tx("delivery: cannot create delivery directory") ) ;
	G_EXCEPTION( MaildirCopyError , tx("delivery: cannot write maildir tmp file") ) ;
	G_EXCEPTION( MaildirMoveError , tx("delivery: cannot move maildir file") ) ;
	struct Config /// A configuration structure for GStore::FileDelivery.
	{
		bool lowercase {true} ; // user-part to mailbox mapping: to lowercase if ascii
		bool hardlink {false} ; // copy the content by hard-linking
		bool no_delete {false} ; // don't delete the original message
	} ;

	FileDelivery( FileStore & , const Config & ) ;
		///< Constructor. The deliver() override will take a ".new" message from
		///< the given file store and deliver it to mailbox sub-directories.

	FileDelivery( FileStore & , const G::Path & to_base_dir , const Config & ) ;
		///< Constructor. The deliver() override will take a ".local" message
		///< from the file store and deliver it to mailboxes that are
		///< sub-directories of the given base directory. If the deliver()
		///< call is for a message that has no ".local" files then deliver()
		///< does nothing.

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
	bool deliverToMailboxes( const G::Path & , const G::Path & , const G::Path & ) ;
	static G::StringArray mailboxes( const Config & , const GStore::Envelope & ) ;
	static std::string mailbox( const Config & , const std::string & ) ;
	static std::string short_( const G::Path & ) ;

private:
	bool m_active ;
	FileStore & m_store ;
	G::Path m_to_base_dir ;
	Config m_config ;
	bool m_local_files ;
} ;

#endif
