//
// Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gspamprocessor.h
///

#ifndef G_SMTP_SPAM_PROCESSOR_H
#define G_SMTP_SPAM_PROCESSOR_H

#include "gdef.h"
#include "gsmtp.h"
#include "gprocessor.h"
#include "gclientptr.h"
#include "gspamclient.h"

/// \namespace GSmtp
namespace GSmtp
{
	class SpamProcessor ;
}

/// \class GSmtp::SpamProcessor
/// A Processor class that passes the body of a
/// message file to a remote process over the network and stores
/// the response back into the file. It looks for a spam
/// header line in the resulting file to determine the
/// overall result.
///
class GSmtp::SpamProcessor : public GSmtp::Processor 
{
public:
	SpamProcessor( const std::string & , unsigned int connection_timeout , unsigned int response_timeout ) ;
		///< Constructor. 

	virtual ~SpamProcessor() ;
		///< Destructor.

	virtual G::Signal1<bool> & doneSignal() ;
		///< Final override from GSmtp::Processor.

	virtual void start( const std::string & path ) ;
		///< Final override from GSmtp::Processor.

	virtual void abort() ;
		///< Final override from GSmtp::Processor.

	virtual std::string text() const ;
		///< Final override from GSmtp::Processor.

	virtual bool cancelled() const ;
		///< Final override from GSmtp::Processor.

	virtual bool repoll() const ;
		///< Final override from GSmtp::Processor.

private:
	SpamProcessor( const SpamProcessor & ) ; // not implemented
	void operator=( const SpamProcessor & ) ; // not implemented
	void clientEvent( std::string , std::string ) ;

private:
	G::Signal1<bool> m_done_signal ;
	GNet::ResolverInfo m_resolver_info ;
	unsigned int m_connection_timeout ;
	unsigned int m_response_timeout ;
	GNet::ClientPtr<SpamClient> m_client ;
	std::string m_text ;
} ;

#endif
