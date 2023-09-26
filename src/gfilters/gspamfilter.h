//
// Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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

#ifndef G_SPAM_FILTER_H
#define G_SPAM_FILTER_H

#include "gdef.h"
#include "gfilter.h"
#include "gfilestore.h"
#include "gclientptr.h"
#include "gspamclient.h"

namespace GFilters
{
	class SpamFilter ;
}

//| \class GFilters::SpamFilter
/// A Filter class that passes the body of a message file to a remote
/// process over the network and optionally stores the response back
/// into the file. It parses the response's "Spam:" header to determine
/// the overall pass/fail result, or it can optionally always pass.
///
class GFilters::SpamFilter : public GSmtp::Filter
{
public:
	SpamFilter( GNet::ExceptionSink , GStore::FileStore & ,
		Filter::Type , const Filter::Config & ,
		const std::string & server_location ,
		bool read_only , bool always_pass ) ;
			///< Constructor.

	~SpamFilter() override ;
		///< Destructor.

private: // overrides
	std::string id() const override ; // GSmtp::Filter
	bool quiet() const override ; // GSmtp::Filter
	G::Slot::Signal<int> & doneSignal() noexcept override ; // GSmtp::Filter
	void start( const GStore::MessageId & ) override ; // GSmtp::Filter
	void cancel() override ; // GSmtp::Filter
	Result result() const override ; // GSmtp::Filter
	std::string response() const override ; // GSmtp::Filter
	int responseCode() const override ; // GSmtp::Filter
	std::string reason() const override ; // GSmtp::Filter
	bool special() const override ; // GSmtp::Filter

public:
	SpamFilter( const SpamFilter & ) = delete ;
	SpamFilter( SpamFilter && ) = delete ;
	SpamFilter & operator=( const SpamFilter & ) = delete ;
	SpamFilter & operator=( SpamFilter && ) = delete ;

private:
	void clientEvent( const std::string & , const std::string & , const std::string & ) ;
	void clientDeleted( const std::string & ) ;
	void done() ;
	void onDoneTimeout() ;

private:
	GNet::ExceptionSink m_es ;
	GNet::Timer<SpamFilter> m_done_timer ;
	G::Slot::Signal<int> m_done_signal ;
	GStore::FileStore & m_file_store ;
	GNet::Location m_location ;
	bool m_read_only ;
	bool m_always_pass ;
	unsigned int m_connection_timeout ;
	unsigned int m_response_timeout ;
	GNet::ClientPtr<GSmtp::SpamClient> m_client_ptr ;
	std::string m_text ;
	Result m_result ;
} ;

#endif
