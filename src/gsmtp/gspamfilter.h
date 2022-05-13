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
/// \file gspamfilter.h
///

#ifndef G_SMTP_SPAM_FILTER_H
#define G_SMTP_SPAM_FILTER_H

#include "gdef.h"
#include "gfilter.h"
#include "gfilestore.h"
#include "gclientptr.h"
#include "gspamclient.h"

namespace GSmtp
{
	class SpamFilter ;
}

//| \class GSmtp::SpamFilter
/// A Filter class that passes the body of a message file to a remote
/// process over the network and optionally stores the response back
/// into the file. It parses the response's "Spam:" header to determine
/// the overall pass/fail result, or it can optionally always pass.
///
class GSmtp::SpamFilter : public Filter
{
public:
	SpamFilter( GNet::ExceptionSink , FileStore & ,
		const std::string & server_location ,
		bool read_only , bool always_pass , unsigned int connection_timeout ,
		unsigned int response_timeout ) ;
			///< Constructor.

	~SpamFilter() override ;
		///< Destructor.

private: // overrides
	std::string id() const override ; // Override from from GSmtp::Filter.
	bool simple() const override ; // Override from from GSmtp::Filter.
	G::Slot::Signal<int> & doneSignal() override ; // Override from from GSmtp::Filter.
	void start( const MessageId & ) override ; // Override from from GSmtp::Filter.
	void cancel() override ; // Override from from GSmtp::Filter.
	bool abandoned() const override ; // Override from from GSmtp::Filter.
	std::string response() const override ; // Override from from GSmtp::Filter.
	std::string reason() const override ; // Override from from GSmtp::Filter.
	bool special() const override ; // Override from from GSmtp::Filter.

public:
	SpamFilter( const SpamFilter & ) = delete ;
	SpamFilter( SpamFilter && ) = delete ;
	void operator=( const SpamFilter & ) = delete ;
	void operator=( SpamFilter && ) = delete ;

private:
	void clientEvent( const std::string & , const std::string & , const std::string & ) ;
	void clientDeleted( const std::string & ) ;
	void emit( bool ) ;

private:
	G::Slot::Signal<int> m_done_signal ;
	GNet::ExceptionSink m_es ;
	FileStore & m_file_store ;
	GNet::Location m_location ;
	bool m_read_only ;
	bool m_always_pass ;
	unsigned int m_connection_timeout ;
	unsigned int m_response_timeout ;
	GNet::ClientPtr<SpamClient> m_client_ptr ;
	std::string m_text ;
} ;

#endif
