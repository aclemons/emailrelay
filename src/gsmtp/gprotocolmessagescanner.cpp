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
		m_scanner_server(scanner_server) ,
		m_scanner_response_timeout(scanner_response_timeout) ,
		m_scanner_connection_timeout(scanner_connection_timeout) ,
		m_id(0UL)
{
	G_DEBUG( "GSmtp::ProtocolMessageScanner::ctor" ) ;
	scannerInit() ;

	// rewire the base class slot/signal
	storageDoneSignal().disconnect() ;
	storageDoneSignal().connect( G::slot(*this,&ProtocolMessageScanner::storageDone) ) ;
}

void GSmtp::ProtocolMessageScanner::scannerInit()
{
	storageDoneSignal().disconnect() ; // base-class signal
	m_scanner_client <<= 
		new ScannerClient(m_scanner_server,m_scanner_connection_timeout,m_scanner_response_timeout) ;
	m_scanner_client->connectedSignal().connect( G::slot(*this,&ProtocolMessageScanner::connectDone) ) ;
	m_scanner_client->doneSignal().connect( G::slot(*this,&ProtocolMessageScanner::scannerDone) ) ;
}

GSmtp::ProtocolMessageScanner::~ProtocolMessageScanner()
{
	storageDoneSignal().disconnect() ; // base-class signal
	if( m_scanner_client.get() ) 
	{
		m_scanner_client->connectedSignal().disconnect() ;
		m_scanner_client->doneSignal().disconnect() ;
	}
}

G::Signal3<bool,bool,std::string> & GSmtp::ProtocolMessageScanner::preparedSignal()
{
	return m_prepared_signal ;
}

bool GSmtp::ProtocolMessageScanner::prepare()
{
	m_scanner_client->startConnecting() ;
	return true ;
}

void GSmtp::ProtocolMessageScanner::connectDone( std::string reason , bool temporary_error )
{
	G_DEBUG( "GSmtp::ProtocolMessageScanner::connectDone: \"" << reason << "\", " << temporary_error ) ;
	m_prepared_signal.emit( reason.empty() , temporary_error , reason ) ;
}

void GSmtp::ProtocolMessageScanner::storageDone( bool , unsigned long id , std::string )
{
	G_DEBUG( "GSmtp::ProtocolMessageScanner::storageDone" ) ;
	m_id = id ;
	FileStore & file_store = dynamic_cast<FileStore&>(m_store) ;
	m_scanner_client->startScanning( file_store.contentPath(id) ) ;
}

void GSmtp::ProtocolMessageScanner::scannerDone( bool /* reason_is_from_scanner */ , std::string reason )
{
	const bool ok = reason.empty() ;
	ProtocolMessageForward::processDone( ok , m_id , reason ) ;
}

void GSmtp::ProtocolMessageScanner::clear()
{
	scannerInit() ;
	Base::clear() ;
}

