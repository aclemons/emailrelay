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
///
/// \file gprotocolmessagestore.h
///

#ifndef G_SMTP_PROTOCOL_MESSAGE_STORE_H
#define G_SMTP_PROTOCOL_MESSAGE_STORE_H

#include "gdef.h"
#include "gsmtp.h"
#include "gprotocolmessage.h"
#include "gmessagestore.h"
#include "gnewmessage.h"
#include "gprocessor.h"
#include "gslot.h"
#include <string>
#include <memory>

/// \namespace GSmtp
namespace GSmtp
{
	class ProtocolMessageStore ;
}

/// \class GSmtp::ProtocolMessageStore
/// A concrete implementation of the
/// ProtocolMessage interface which stores incoming
/// messages in the message store.
/// \see GSmtp::ProtocolMessageForward
///
class GSmtp::ProtocolMessageStore : public GSmtp::ProtocolMessage 
{
public:
	ProtocolMessageStore( MessageStore & store , const G::Executable & newfile_preprocessor ) ;
		///< Constructor.

	virtual ~ProtocolMessageStore() ;
		///< Destructor.

	virtual G::Signal3<bool,unsigned long,std::string> & doneSignal() ;
		///< See ProtocolMessage.

	virtual G::Signal3<bool,bool,std::string> & preparedSignal() ;
		///< See ProtocolMessage.

	virtual void clear() ;
		///< See ProtocolMessage.

	virtual bool setFrom( const std::string & from_user ) ;
		///< See ProtocolMessage.

	virtual bool prepare() ;
		///< See ProtocolMessage.

	virtual bool addTo( const std::string & to_user , Verifier::Status to_status ) ;
		///< See ProtocolMessage.

	virtual void addReceived( const std::string & ) ;
		///< See ProtocolMessage.

	virtual void addText( const std::string & ) ;
		///< See ProtocolMessage.

	virtual std::string from() const ;
		///< See ProtocolMessage.

	virtual void process( const std::string & auth_id , const std::string & client_ip ) ;
		///< See ProtocolMessage.

private:
	void operator=( const ProtocolMessageStore & ) ; // not implemented
	void preprocessorDone( bool ) ;

private:
	MessageStore & m_store ;
	Processor m_newfile_preprocessor ;
	std::auto_ptr<NewMessage> m_msg ;
	std::string m_from ;
	G::Signal3<bool,unsigned long,std::string> m_done_signal ;
	G::Signal3<bool,bool,std::string> m_prepared_signal ;
} ;

#endif

