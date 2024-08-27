//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gfiledelivery.cpp
///

#include "gdef.h"
#include "gfiledelivery.h"
#include "gfilestore.h"
#include "grange.h"
#include "gfile.h"
#include "gscope.h"
#include "gstringarray.h"
#include "gprocess.h"
#include "ghostname.h"
#include "gexception.h"
#include "gassert.h"
#include "glog.h"
#include <algorithm>
#include <sstream>

GStore::FileDelivery::FileDelivery( FileStore & store , const Config & config ) :
	m_store(store) ,
	m_config(config)
{
}

bool GStore::FileDelivery::deliver( const MessageId & message_id , bool is_new )
{
	bool deleted = false ;
	FileStore::State store_state = is_new ? FileStore::State::New : FileStore::State::Locked ;
	G::Path envelope_path = epath( message_id , store_state ) ;
	Envelope envelope = FileStore::readEnvelope( envelope_path ) ;
	if( !envelope.to_local.empty() )
	{
		if( m_store.directory() != m_store.deliveryDir() )
		{
			G_LOG( "GStore::FileDelivery::deliver: delivery: delivering " << message_id.str() << " to "
				"[" << m_store.deliveryDir() << "/<mbox>]" ) ;
		}

		deleted = deliverToMailboxes( m_store.deliveryDir() , envelope , envelope_path , cpath(message_id) ) ;
	}
	return deleted ;
}

bool GStore::FileDelivery::deliverToMailboxes( const G::Path & delivery_dir , const Envelope & envelope ,
	const G::Path & envelope_path , const G::Path & content_path )
{
	G_ASSERT( !envelope.to_local.empty() ) ;

	// map recipient addresses to mailbox names
	G::StringArray mailbox_list = mailboxes( m_config , envelope ) ;

	// process each mailbox
	for( const auto & mailbox : mailbox_list )
	{
		// create the target directory if necessary
		// (see also GPop::Store::prepare())
		//
		if( mailbox.empty() || !G::Str::isPrintable(mailbox) || !G::Path(mailbox).simple() )
			throw MkdirError( "invalid mailbox name" , G::Str::printable(mailbox) ) ;
		G::Path mbox_dir = delivery_dir/mailbox ;
		if( !FileOp::isdir(mbox_dir) )
		{
			G_LOG( "GStore::FileDelivery::deliverToMailboxes: delivery: creating mailbox [" << mailbox << "]" ) ;
			if( !FileOp::mkdir( mbox_dir ) )
				throw MkdirError( mbox_dir.str() , G::Process::strerror(FileOp::errno_()) ) ;
		}

		// copy files
		deliverTo( m_store , "deliver" , mbox_dir , envelope_path , content_path , m_config.hardlink ) ;
	}

	// delete the original files if no remote recipients
	if( envelope.to_remote.empty() && !m_config.no_delete )
	{
		FileOp::remove( content_path ) ;
		FileOp::remove( envelope_path ) ;
		return true ;
	}
	else
	{
		return false ;
	}
}

void GStore::FileDelivery::deliverTo( FileStore & /*store*/ , std::string_view prefix ,
	const G::Path & dst_dir , const G::Path & envelope_path , const G::Path & content_path ,
	bool hardlink , bool pop_by_name )
{
	if( FileOp::isdir( dst_dir/"tmp" , dst_dir/"cur" , dst_dir/"new" ) )
	{
		// copy content to maildir's "new" sub-directory via "tmp"
		static int seq {} ;
		std::ostringstream ss ;
		ss << G::SystemTime::now() << "." << G::Process::Id().str() << "." << hostname() << "." << seq++ ;
		G::Path tmp_content_path = dst_dir/"tmp"/ss.str() ;
		G::Path new_content_path = dst_dir/"new"/ss.str() ;
		if( !FileOp::copy( content_path , tmp_content_path , hardlink ) )
			throw MaildirCopyError( prefix , tmp_content_path.str() , G::Process::strerror(FileOp::errno_()) ) ;
		if( !FileOp::rename( tmp_content_path , new_content_path ) )
			throw MaildirMoveError( prefix , new_content_path.str() , G::Process::strerror(FileOp::errno_()) ) ;
		G_DEBUG( "GStore::FileDelivery::deliverTo: delivery: delivered " << id(envelope_path) << " as maildir " << ss.str() ) ;
	}
	else if( pop_by_name )
	{
		// envelope only
		std::string new_filename = content_path.withoutExtension().basename() ;
		G::Path new_envelope_path = dst_dir / (new_filename+".envelope") ;
		if( !FileOp::copy( envelope_path , new_envelope_path ) )
			throw EnvelopeWriteError( prefix , new_envelope_path.str() , G::Process::strerror(FileOp::errno_()) ) ;
	}
	else
	{
		std::string new_filename = content_path.withoutExtension().basename() ;
		G::Path new_content_path = dst_dir / (new_filename+".content") ;
		G::Path new_envelope_path = dst_dir / (new_filename+".envelope") ;
		G::ScopeExit clean_up_content( [new_content_path](){FileOp::remove(new_content_path);} ) ;

		// copy or link the content -- maybe edit to add "Delivered-To" etc?
		bool ok = FileOp::copy( content_path , new_content_path , hardlink ) ;
		if( !ok )
			throw ContentWriteError( prefix , new_content_path.str() , G::Process::strerror(FileOp::errno_()) ) ;

		// copy the envelope -- maybe remove other recipients, but no need
		if( !FileOp::copy( envelope_path , new_envelope_path ) )
			throw EnvelopeWriteError( prefix , new_envelope_path.str() , G::Process::strerror(FileOp::errno_()) ) ;

		clean_up_content.release() ;
		G_DEBUG( "GStore::FileDelivery::deliver: " << prefix << ": delivered " << id(envelope_path) << " to mailbox " << dst_dir.basename() ) ;
	}
}

G::StringArray GStore::FileDelivery::mailboxes( const Config & config , const GStore::Envelope & envelope )
{
	G_ASSERT( !envelope.to_local.empty() ) ;
	using namespace std::placeholders ;
	G::StringArray list ;
	std::transform( envelope.to_local.begin() , envelope.to_local.end() ,
		std::back_inserter(list) , std::bind(&FileDelivery::mailbox,config,_1) ) ;
	std::sort( list.begin() , list.end() ) ;
	list.erase( std::unique( list.begin() , list.end() ) , list.end() ) ;
	G_ASSERT( !list.empty() ) ;
	return list ;
}

std::string GStore::FileDelivery::mailbox( const Config & , const std::string & recipient )
{
	// we are only delivering for local recipients where the address verifier
	// has already mapped the recipient address to a nice mailbox name,
	// so this is a no-op
	//
	const std::string & mailbox = recipient ;

	G_LOG( "GStore::FileDelivery::mailbox: delivery: recipient [" << recipient << "]: delivery to mailbox [" << mailbox << "]" ) ;
	return mailbox ;
}

std::string GStore::FileDelivery::id( const G::Path & envelope_path )
{
	return envelope_path.withoutExtension().basename() ;
}

G::Path GStore::FileDelivery::epath( const MessageId & message_id , FileStore::State store_state ) const
{
	return m_store.envelopePath( message_id , store_state ) ;
}

G::Path GStore::FileDelivery::cpath( const MessageId & message_id ) const
{
	return m_store.contentPath( message_id ) ;
}

std::string GStore::FileDelivery::hostname()
{
	std::string name = G::hostname() ;
	if( name.empty() ) name = "localhost" ;
	G::Str::replace( name , '/' , '_' ) ;
	G::Str::replace( name , '\\' , '_' ) ;
	G::Str::replace( name , '.' , '_' ) ;
	return name ;
}

