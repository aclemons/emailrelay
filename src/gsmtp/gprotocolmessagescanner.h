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
// gprotocolmessagescanner.h
//

#ifndef G_SMTP_PROTOCOL_MESSAGE_SCANNER_H
#define G_SMTP_PROTOCOL_MESSAGE_SCANNER_H

#include "gdef.h"
#include "gsmtp.h"
#include "gprotocolmessage.h"
#include "gprotocolmessagestore.h"
#include "gprotocolmessageforward.h"
#include "gexe.h"
#include "gscannerclient.h"
#include "gsecrets.h"
#include "gsmtpclient.h"
#include "gmessagestore.h"
#include "gnewmessage.h"
#include <string>
#include <memory>

namespace GSmtp
{
	class ProtocolMessageScanner ;
}

// Class: GSmtp::ProtocolMessageScanner
// Description: A derivation of ProtocolMessageForward which adds in
// a scanning step.
// 
// The scanning part deletages to a ScannerClient data member.
//
// See also: GSmtp::ProtocolMessageStore, GSmtp::ProtocolMessageForward
//
class GSmtp::ProtocolMessageScanner : public GSmtp::ProtocolMessageForward 
{
public:
	ProtocolMessageScanner( MessageStore & store , 
		const G::Executable & newfile_preprocessor ,
		const GSmtp::Client::Config & client_config ,
		const Secrets & client_secrets , 
		const std::string & smtp_server_address , 
		unsigned int smtp_connection_timeout ,
		const std::string & scanner_server_address , 
		unsigned int scanner_response_timeout , unsigned int scanner_connection_timeout ) ;
			// Constructor. The 'store' and 'client-secrets' references
			// are kept.

	virtual ~ProtocolMessageScanner() ;
		// Destructor.

	virtual G::Signal3<bool,bool,std::string> & preparedSignal() ;
		// See ProtocolMessage.

	virtual bool prepare() ;
		// See ProtocolMessage.

	virtual void clear() ;
		// See ProtocolMessage.

private:
	void operator=( const ProtocolMessageScanner & ) ; // not implemented
	void connectDone( std::string reason , bool ) ;
	void storageDone( bool success , unsigned long id , std::string reason ) ;
	void scannerDone( bool b , std::string reason ) ;
	void scannerInit() ;

private:
	typedef ProtocolMessageForward Base ;
	MessageStore & m_store ;
	std::string m_scanner_server ;
	unsigned int m_scanner_response_timeout ;
	unsigned int m_scanner_connection_timeout ;
	std::auto_ptr<ScannerClient> m_scanner_client ;
	G::Signal3<bool,bool,std::string> m_prepared_signal ;
	unsigned long m_id ;
} ;

#endif
