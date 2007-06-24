//
// Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gprotocolmessagescanner.cpp
//

#include "gdef.h"
#include "gsmtp.h"
#include "gprotocolmessagescanner.h"
#include "gmessagestore.h"
#include "gfilestore.h"
#include "gmemory.h"
#include "gstr.h"
#include "gassert.h"
#include "glog.h"

GSmtp::ProtocolMessageScanner::ProtocolMessageScanner( MessageStore & store , 
	const G::Executable & newfile_preprocessor ,
	const GSmtp::Client::Config & client_config ,
	const Secrets & client_secrets , 
	const std::string & smtp_server ,
	unsigned int smtp_connection_timeout ,
	const std::string & scanner_server ,
	unsigned int scanner_response_timeout , unsigned int scanner_connection_timeout ) :
		ProtocolMessageForward(store,newfile_preprocessor,client_config,
			client_secrets,smtp_server,smtp_connection_timeout),
		m_store(store) ,
		m_scanner_resolver_info(scanner_server) ,
		m_scanner_response_timeout(scanner_response_timeout) ,
		m_scanner_connection_timeout(scanner_connection_timeout) ,
		m_scanner_client(NULL,true) ,
		m_id(0UL)
{
	G_DEBUG( "GSmtp::ProtocolMessageScanner::ctor" ) ;

	m_scanner_client.reset( new ScannerClient(m_scanner_resolver_info,
		m_scanner_connection_timeout,m_scanner_response_timeout) ) ;

	m_scanner_client.eventSignal().connect( G::slot(*this,&ProtocolMessageScanner::scannerEvent) ) ;
	m_scanner_client.doneSignal().connect( G::slot(*this,&ProtocolMessageScanner::scannerDone) ) ;

	// rewire the base class slot/signal so that the storage 'done' signal
	// is delivered to this class
	//
	storageDoneSignal().disconnect() ;
	storageDoneSignal().connect( G::slot(*this,&ProtocolMessageScanner::storageDone) ) ;
}

GSmtp::ProtocolMessageScanner::~ProtocolMessageScanner()
{
	storageDoneSignal().disconnect() ; // base-class signal
	m_scanner_client.eventSignal().disconnect() ;
	m_scanner_client.doneSignal().disconnect() ;
}

void GSmtp::ProtocolMessageScanner::storageDone( bool , unsigned long id , std::string )
{
	G_DEBUG( "GSmtp::ProtocolMessageScanner::storageDone" ) ;
	m_id = id ;
	FileStore & file_store = dynamic_cast<FileStore&>(m_store) ;

	if( m_scanner_client.get() == NULL )
		m_scanner_client.reset( new ScannerClient(m_scanner_resolver_info,
			m_scanner_connection_timeout,m_scanner_response_timeout) ) ;

	m_scanner_client->startScanning( file_store.contentPath(id) ) ;
}

void GSmtp::ProtocolMessageScanner::scannerEvent( std::string s1 , std::string s2 )
{
	if( s1 == "scanner" )
	{
		const bool ok = s2.empty() ;
		ProtocolMessageForward::processDone( ok , m_id , s2 ) ;
	}
}

void GSmtp::ProtocolMessageScanner::scannerDone( std::string reason , bool )
{
	const bool ok = false ;
	ProtocolMessageForward::processDone( ok , m_id , reason ) ;
}

void GSmtp::ProtocolMessageScanner::clear()
{
	m_scanner_client.reset() ;
	Base::clear() ;
}

/// \file gprotocolmessagescanner.cpp
