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
#include "gfile.h"
#include "gscope.h"
#include "gstringarray.h"
#include "gexception.h"
#include "gassert.h"
#include "glog.h"
#include <algorithm>

GStore::FileDelivery::FileDelivery( FileStore & store , const std::string & domain ) :
	m_active(true) ,
	m_store(store) ,
	m_domain(domain) ,
	m_local_files(false)
{
}

GStore::FileDelivery::FileDelivery( FileStore & store , const std::string & domain , const G::Path & dst ) :
	m_active(!dst.empty()) ,
	m_store(store) ,
	m_domain(domain) ,
	m_dst(dst) ,
	m_local_files(!dst.empty())
{
}

void GStore::FileDelivery::deliver( const MessageId & message_id )
{
	if( m_active )
	{
		G_ASSERT( !m_domain.empty() ) ;
		G::Path envelope_path = m_store.envelopePath( message_id , GStore::FileStore::State::New ) ;
		G::Path content_path = m_store.contentPath( message_id ) ;
		G::Path base_dir = m_dst.empty() ? content_path.dirname() : m_dst ;

		if( m_local_files )
		{
			envelope_path = envelope_path.withoutExtension().str().append(".local") ;
			content_path = content_path.str().append(".local") ;
			G_LOG( "GStore::FileDelivery::deliver: delivery: delivering " << short_(envelope_path) << " to [" << m_dst.basename() << "]" ) ;
		}
		else
		{
			G_LOG( "GStore::FileDelivery::deliver: delivery: delivering " << short_(envelope_path) ) ;
		}

		deliverImp( envelope_path , content_path , base_dir ) ;
		G_LOG( "GStore::FileDelivery::deliver: delivery: delivered " << short_(envelope_path) ) ;

		// delete once fully delivered
		G::File::remove( content_path , std::nothrow ) ;
		G::File::remove( envelope_path , std::nothrow ) ;
	}
}

void GStore::FileDelivery::deliverImp( const G::Path & envelope_path , const G::Path & content_path ,
	const G::Path & base_dir )
{
	GStore::Envelope envelope = readEnvelope( envelope_path ) ;

	// normalise and validate the recipient addresses -- valid
	// addresses become simple mailbox names and invalid addresses
	// are mapped to "postmaster"
	//
	G::StringArray mailbox_list = mailboxes( envelope , m_domain ) ;
	G_ASSERT( !mailbox_list.empty() ) ;

	// process each mailbox
	for( const auto & mailbox : mailbox_list )
	{
		// prepare a target directory
		G::Path dst_dir = base_dir + mailbox ;
		if( !G::File::isDirectory( dst_dir , std::nothrow ) )
		{
			int e = mkdir( dst_dir ) ;
			if( e )
				throw G::Exception( "delivery: cannot create delivery directory for [" + mailbox + "]" , G::Process::strerror(e) ) ;
		}

		// prepare new message file paths
		std::string new_filename = m_store.newId().str() ;
		G::Path new_content_path = dst_dir + (new_filename+".content") ;
		G::Path new_envelope_path = dst_dir + (new_filename+".envelope.new") ;

		G::ScopeExit clean_up_content( [new_content_path](){G::File::remove(new_content_path,std::nothrow);} ) ;
		G::ScopeExit clean_up_envelope( [new_envelope_path](){G::File::remove(new_envelope_path,std::nothrow);} ) ;

		// link or copy the content
		// TODO optionally add "Delivered-To" header
		{
			bool linked = hardlink( content_path , new_content_path ) ;
			if( !linked )
			{
				std::ifstream content_in ;
				std::ofstream content_out ;
				if( !openIn( content_in , content_path ) || !openOut( content_out , new_content_path ) )
					throw G::Exception( "delivery: cannot copy content file" , content_path.str() , new_content_path.str() ) ;
				G::File::copy( content_in , content_out ) ;
				content_out.close() ;
				if( !content_out )
					throw G::Exception( "delivery: cannot write content file" , new_content_path.str() ) ;
			}
		}

		// copy the envelope
		GStore::Envelope new_envelope = envelope ;
		new_envelope.to_local.clear() ;
		new_envelope.to_remote.clear() ;
		new_envelope.to_remote.push_back( mailbox ) ;
		GStore::Envelope::copy( new_envelope , new_envelope_path , envelope ) ;

		// commit
		rename( new_envelope_path , new_envelope_path.withoutExtension() ) ;
		clean_up_content.release() ;
		clean_up_envelope.release() ;
	}
}

G::StringArray GStore::FileDelivery::mailboxes( const GStore::Envelope & envelope , const std::string & this_domain )
{
	using namespace std::placeholders ;
	G::StringArray list ;
	std::transform( envelope.to_remote.begin() , envelope.to_remote.end() ,
		std::back_inserter(list) , std::bind(&FileDelivery::mailbox,this_domain,_1) ) ;
	std::transform( envelope.to_local.begin() , envelope.to_local.end() ,
		std::back_inserter(list) , std::bind(&FileDelivery::mailbox,this_domain,_1) ) ;
	std::sort( list.begin() , list.end() ) ;
	list.erase( std::unique( list.begin() , list.end() ) , list.end() ) ;
	return list ;
}

std::string GStore::FileDelivery::mailbox( const std::string & this_domain , const std::string & recipient )
{
	std::string user = normalise( G::Str::head( recipient , "@" , false ) ) ;
	std::string domain = normalise( G::Str::tail( recipient , "@" ) ) ;
	bool user_ok = user == "postmaster" || lookup( user ) ;
	if( domain != this_domain )
	{
		G_LOG( "GStore::FileDelivery::mailbox: delivery: recipient [" << recipient << "]: "
			<< "invalid domain (not [" << this_domain << "])"
			<< (user_ok?"":" and invalid user") << ": delivery to postmaster" ) ;
		user = "postmaster" ;
	}
	else if( !user_ok )
	{
		G_LOG( "GStore::FileDelivery::mailbox: delivery: recipient [" << recipient << "]: invalid user: delivery to postmaster" ) ;
		user = "postmaster" ;
	}
	else
	{
		G_LOG( "GStore::FileDelivery::mailbox: delivery: recipient [" << recipient << "]: delivery to [" << user << "]" ) ;
	}
	return user ;
}

bool GStore::FileDelivery::lookup( const std::string & user )
{
	uid_t uid = 0 ;
	gid_t gid = 0 ;
	return G::Identity::lookupUser( user , uid , gid ) ;
}

int GStore::FileDelivery::mkdir( const G::Path & dir )
{
	GStore::FileWriter claim_root ;
	int e = 0 ;
	if( !G::File::mkdir( dir , std::nothrow ) )
		e = G::Process::errno_() ;
	return e ;
}

GStore::Envelope GStore::FileDelivery::readEnvelope( const G::Path & envelope_path )
{
	GStore::Envelope envelope ;
	std::ifstream envelope_stream ;
	{
		G::Root claim_root ;
		G::File::open( envelope_stream , envelope_path ) ;
	}
	if( !envelope_stream )
		throw G::Exception( "delivery: cannot open envelope file" , envelope_path.str() ) ;
	GStore::Envelope::read( envelope_stream , envelope ) ;
	return envelope ;
}

bool GStore::FileDelivery::hardlink( const G::Path & src , const G::Path & dst )
{
	GStore::FileWriter claim_root ;
	return G::File::hardlink( src , dst , std::nothrow ) ;
}

bool GStore::FileDelivery::openIn( std::ifstream & in , const G::Path & path )
{
	G::Root claim_root ;
	G::File::open( in , path ) ;
	return !in.fail() ;
}

bool GStore::FileDelivery::openOut( std::ofstream & out , const G::Path & path )
{
	GStore::FileWriter claim_root ;
	G::File::open( out , path ) ;
	return !out.fail() ;
}

void GStore::FileDelivery::rename( const G::Path & src , const G::Path & dst )
{
	GStore::FileWriter claim_root ;
	G::File::rename( src , dst ) ;
}

std::string GStore::FileDelivery::normalise( const std::string & s )
{
	return G::Str::isPrintableAscii(s) ? G::Str::lower(s) : s ;
}

std::string GStore::FileDelivery::short_( const G::Path & path )
{
	return path.simple() ? path.str() : G::Path(path.dirname().basename(),path.basename()).str() ;
}

