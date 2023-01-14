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

GStore::FileDelivery::FileDelivery( FileStore & store , const std::string & domain , std::pair<int,int> uid_range ) :
	m_active(true) ,
	m_store(store) ,
	m_domain(domain) ,
	m_uid_range(uid_range) ,
	m_local_files(false)
{
}

GStore::FileDelivery::FileDelivery( FileStore & store , const std::string & domain , const G::Path & base_dir ) :
	m_active(!base_dir.empty()) ,
	m_store(store) ,
	m_domain(domain) ,
	m_uid_range(0,-1) ,
	m_base_dir(base_dir) ,
	m_local_files(!base_dir.empty())
{
}

void GStore::FileDelivery::deliver( const MessageId & message_id )
{
	if( m_active )
	{
		G_ASSERT( !m_domain.empty() ) ;
		G::Path envelope_path = m_store.envelopePath( message_id , GStore::FileStore::State::New ) ;
		G::Path content_path = m_store.contentPath( message_id ) ;
		G::Path base_dir = m_base_dir.empty() ? content_path.dirname() : m_base_dir ;

		if( m_local_files )
		{
			envelope_path = envelope_path.withoutExtension().str().append(".local") ;
			if( !FileOp::exists( envelope_path ) )
				return ; // no-op if no ".local" envelope file, ie. no local recipients

			content_path = content_path.str().append(".local") ;
			G_LOG( "GStore::FileDelivery::deliver: delivery: delivering " << short_(envelope_path) << " to [" << m_base_dir.basename() << "]" ) ;
		}
		else
		{
			G_LOG( "GStore::FileDelivery::deliver: delivery: delivering " << short_(envelope_path) ) ;
		}

		if( deliverToMailboxes( base_dir , envelope_path , content_path , m_uid_range ) )
		{
			// delete once fully delivered
			FileOp::remove( content_path ) ;
			FileOp::remove( envelope_path ) ;
		}
	}
}

bool GStore::FileDelivery::deliverToMailboxes( const G::Path & base_dir , const G::Path & envelope_path ,
	const G::Path & content_path , std::pair<int,int> uid_range )
{
	GStore::Envelope envelope = FileStore::readEnvelope( envelope_path ) ;

	// normalise and validate the recipient addresses -- valid
	// addresses become simple mailbox names and invalid addresses
	// are mapped to "postmaster"
	//
	G::StringArray mailbox_list = mailboxes( envelope , m_domain , uid_range ) ;
	G_ASSERT( !mailbox_list.empty() ) ;

	// process each mailbox
	for( const auto & mailbox : mailbox_list )
	{
		// create the target directory if necessary
		G::Path mbox_dir = base_dir + mailbox ;
		if( !FileOp::isdir(mbox_dir) )
		{
			G_LOG( "GStore::FileDelivery::deliverToMailboxes: delivery: creating mailbox directory for [" << mailbox << "]" ) ;
			if( !FileOp::mkdir( mbox_dir ) )
				throw MkdirError( mbox_dir.str() , G::Process::strerror(FileOp::errno_()) ) ;
		}

		// copy files
		deliverTo( m_store , mbox_dir , envelope_path , content_path ) ;
	}
	return !mailbox_list.empty() ;
}

void GStore::FileDelivery::deliverTo( FileStore & /*store*/ , const G::Path & mbox_dir ,
	const G::Path & envelope_path , const G::Path & content_path , bool hardlink )
{
	if( FileOp::isdir( mbox_dir+"tmp" , mbox_dir+"cur" , mbox_dir+"new" ) )
	{
		// copy content to maildir "new"
		static int seq {} ;
		std::ostringstream ss ;
		ss << G::SystemTime::now() << "." << G::Process::Id().str() << "." << G::hostname() << "." << seq++ ;
		G::Path tmp_content_path = mbox_dir + "tmp" + ss.str() ;
		G::Path new_content_path = mbox_dir + "new" + ss.str() ;
		if( !FileOp::copy( content_path , tmp_content_path , hardlink ) )
			throw MaildirCopyError( tmp_content_path.str() , G::Process::strerror(FileOp::errno_()) ) ;
		if( !FileOp::rename( tmp_content_path , new_content_path ) )
			throw MaildirMoveError( new_content_path.str() , G::Process::strerror(FileOp::errno_()) ) ;
		G_DEBUG( "GStore::FileDelivery::deliverTo: delivery: delivered " << short_(new_content_path) ) ;
	}
	else
	{
		//std::string new_filename = store.newId().str() ;
		std::string new_filename = content_path.withoutExtension().basename() ;
		G::Path new_content_path = mbox_dir + (new_filename+".content") ;
		G::Path new_envelope_path = mbox_dir + (new_filename+".envelope") ;
		G::ScopeExit clean_up_content( [new_content_path](){FileOp::remove(new_content_path);} ) ;

		// link the content -- but maybe copy and edit to add "Delivered-To" etc?
		bool ok = FileOp::copy( content_path , new_content_path , hardlink ) ;
		if( !ok )
				throw ContentWriteError( new_content_path.str() , G::Process::strerror(FileOp::errno_()) ) ;

		// copy the envelope -- maybe remove other recipients, but no need
		if( !FileOp::copy( envelope_path , new_envelope_path ) )
			throw EnvelopeWriteError( new_envelope_path.str() , G::Process::strerror(FileOp::errno_()) ) ;

		clean_up_content.release() ;
		G_DEBUG( "GStore::FileDelivery::deliver: delivery: delivered " << short_(new_content_path) ) ;
	}
}

G::StringArray GStore::FileDelivery::mailboxes( const GStore::Envelope & envelope , const std::string & this_domain ,
	std::pair<int,int> uid_range )
{
	using namespace std::placeholders ;
	G::StringArray list ;
	std::transform( envelope.to_remote.begin() , envelope.to_remote.end() ,
		std::back_inserter(list) , std::bind(&FileDelivery::mailbox,this_domain,uid_range,_1) ) ;
	std::transform( envelope.to_local.begin() , envelope.to_local.end() ,
		std::back_inserter(list) , std::bind(&FileDelivery::mailbox,this_domain,uid_range,_1) ) ;
	std::sort( list.begin() , list.end() ) ;
	list.erase( std::unique( list.begin() , list.end() ) , list.end() ) ;
	return list ;
}

std::string GStore::FileDelivery::mailbox( const std::string & this_domain , std::pair<int,int> uid_range ,
	const std::string & recipient )
{
	std::string user = normalise( G::Str::head( recipient , "@" , false ) ) ;
	std::string domain = normalise( G::Str::tail( recipient , "@" ) ) ;
	bool user_ok = user == "postmaster" || lookup( user , uid_range ) ;
	if( domain != this_domain )
	{
		G_LOG( "GStore::FileDelivery::mailbox: delivery: recipient [" << recipient << "]: "
			<< "delivery to [postmaster] ("
			<< "domain not [" << this_domain << "]"
			<< (user_ok?"":" and invalid user")
			<< ")" ) ;
		user = "postmaster" ;
	}
	else if( !user_ok )
	{
		G_LOG( "GStore::FileDelivery::mailbox: delivery: recipient [" << recipient << "]: "
			<< "delivery to [postmaster] (invalid user)" ) ;
		user = "postmaster" ;
	}
	else
	{
		G_LOG( "GStore::FileDelivery::mailbox: delivery: recipient [" << recipient << "]: delivery to [" << user << "]" ) ;
	}
	return user ;
}

bool GStore::FileDelivery::lookup( const std::string & user , std::pair<int,int> uid_range )
{
	using namespace G::Range ;
	uid_t uid = 0 ;
	gid_t gid = 0 ;
	return G::Identity::lookupUser( user , uid , gid ) && within( uid_range , uid ) ;
}

std::string GStore::FileDelivery::normalise( const std::string & s )
{
	return G::Str::isPrintableAscii(s) ? G::Str::lower(s) : s ;
}

std::string GStore::FileDelivery::short_( const G::Path & path )
{
	return path.simple() ? path.str() : G::Path(path.dirname().basename(),path.basename()).str() ;
}

