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
// gprotocolmessagestore.cpp
//

#include "gdef.h"
#include "gsmtp.h"
#include "gprotocolmessagestore.h"
#include "gmessagestore.h"
#include "gmemory.h"
#include "gstr.h"
#include "gassert.h"
#include "glog.h"

GSmtp::ProtocolMessageStore::ProtocolMessageStore( MessageStore & store , 
	const G::Executable & newfile_preprocessor_exe ) :
		m_store(store) ,
		m_newfile_preprocessor(newfile_preprocessor_exe)
{
	m_newfile_preprocessor.doneSignal().connect( G::slot(*this,&ProtocolMessageStore::preprocessorDone) ) ;
}

GSmtp::ProtocolMessageStore::~ProtocolMessageStore()
{
	m_newfile_preprocessor.doneSignal().disconnect() ;
}

void GSmtp::ProtocolMessageStore::clear()
{
	m_msg <<= 0 ;
	m_from.erase() ;
	m_newfile_preprocessor.abort() ;
}

bool GSmtp::ProtocolMessageStore::setFrom( const std::string & from )
{
	try
	{
		if( from.length() == 0U ) // => probably a failure notification message
			G_WARNING( "GSmtp::ProtocolMessageStore: empty MAIL-FROM return path" ) ;

		G_ASSERT( m_msg.get() == NULL ) ;
		clear() ; // just in case

		std::auto_ptr<NewMessage> new_message( m_store.newMessage(from) ) ;
		m_msg <<= new_message.release() ;

		m_from = from ;
		return true ;
	}
	catch( std::exception & e )
	{
		G_ERROR( "GSmtp::ProtocolMessage::setFrom: exception: " << e.what() ) ;
		return false ;
	}
}

bool GSmtp::ProtocolMessageStore::prepare()
{
	return false ; // no async preparation required for this class
}

bool GSmtp::ProtocolMessageStore::addTo( const std::string & to , Verifier::Status to_status )
{
	G_ASSERT( m_msg.get() != NULL ) ;
	if( to.length() > 0U && m_msg.get() != NULL )
	{
		if( !to_status.is_valid )
		{
			G_WARNING( "GSmtp::ProtocolMessage: rejecting recipient \"" <<to << "\": "<< to_status.reason << ": " << to_status.help );
			return false ;
		}
		else
		{
			m_msg->addTo( to_status.address , to_status.is_local ) ;
			return true ;
		}
	}
	else
	{
		return false ;
	}
}

void GSmtp::ProtocolMessageStore::addReceived( const std::string & line )
{
	addText( line ) ;
}

void GSmtp::ProtocolMessageStore::addText( const std::string & line )
{
	G_ASSERT( m_msg.get() != NULL ) ;
	if( m_msg.get() != NULL )
		m_msg->addText( line ) ;
}

std::string GSmtp::ProtocolMessageStore::from() const
{
	return m_msg.get() ? m_from : std::string() ;
}

void GSmtp::ProtocolMessageStore::process( const std::string & auth_id , const std::string & client_ip )
{
	try
	{
		G_ASSERT( m_msg.get() != NULL ) ;
		if( m_msg.get() == NULL )
			throw G::Exception( "internal error" ) ; // never gets here

		// write ".new" envelope
		std::string message_location = m_msg->prepare( auth_id , client_ip ) ; 

		// start preprocessing
		m_newfile_preprocessor.start( message_location ) ;
	}
	catch( std::exception & e )
	{
		G_ERROR( "GSmtp::ProtocolMessage::process: exception: " << e.what() ) ;
		clear() ;
		m_done_signal.emit( false , 0UL , e.what() ) ;
	}
}

void GSmtp::ProtocolMessageStore::preprocessorDone( bool ok )
{
	try
	{
		G_ASSERT( m_msg.get() != NULL ) ;
		if( m_msg.get() == NULL )
			throw G::Exception( "internal error" ) ; // never gets here

		unsigned long id = 0UL ;
		if( ok )
		{
			m_msg->commit() ;
			id = m_msg->id() ;
		}
		else if( ! m_newfile_preprocessor.cancelled() )
		{
			std::string reason = m_newfile_preprocessor.text("pre-processing failed") ;
			throw ProtocolMessage::ProcessingError( reason ) ;
		}
		if( m_newfile_preprocessor.repoll() )
		{
			m_store.repoll() ;
		}
		clear() ;
		m_done_signal.emit( true , id , std::string() ) ;
	}
	catch( std::exception & e )
	{
		G_ERROR( "GSmtp::ProtocolMessage::preprocessorDone: exception: " << e.what() ) ;
		clear() ;
		m_done_signal.emit( false , 0UL , e.what() ) ;
	}
}

G::Signal3<bool,unsigned long,std::string> & GSmtp::ProtocolMessageStore::doneSignal()
{
	return m_done_signal ;
}

G::Signal3<bool,bool,std::string> & GSmtp::ProtocolMessageStore::preparedSignal()
{
	return m_prepared_signal ;
}

