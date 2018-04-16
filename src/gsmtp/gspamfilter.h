//
// Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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

#ifndef G_SMTP_SPAM_FILTER__H
#define G_SMTP_SPAM_FILTER__H

#include "gdef.h"
#include "gsmtp.h"
#include "gfilter.h"
#include "gclientptr.h"
#include "gspamclient.h"

namespace GSmtp
{
	class SpamFilter ;
}

/// \class GSmtp::SpamFilter
/// A Filter class that passes the body of a message file to a remote
/// process over the network and stores the response back into the
/// file. It looks for a spam header line in the resulting file to
/// determine the overall result.
///
class GSmtp::SpamFilter : public Filter
{
public:
	SpamFilter( GNet::ExceptionHandler & ,
		bool server_side , const std::string & server_location ,
		unsigned int connection_timeout , unsigned int response_timeout ) ;
			///< Constructor.

	virtual ~SpamFilter() ;
		///< Destructor.

	virtual std::string id() const override ;
		///< Override from from GSmtp::Filter.

	virtual bool simple() const override ;
		///< Override from from GSmtp::Filter.

	virtual G::Slot::Signal1<bool> & doneSignal() override ;
		///< Override from from GSmtp::Filter.

	virtual void start( const std::string & path ) override ;
		///< Override from from GSmtp::Filter.

	virtual void cancel() override ;
		///< Override from from GSmtp::Filter.

	virtual std::string text() const override ;
		///< Override from from GSmtp::Filter.

	virtual bool specialCancelled() const override ;
		///< Override from from GSmtp::Filter.

	virtual bool specialOther() const override ;
		///< Override from from GSmtp::Filter.

private:
	SpamFilter( const SpamFilter & ) ; // not implemented
	void operator=( const SpamFilter & ) ; // not implemented
	void clientEvent( std::string , std::string ) ;
	void emit( bool ) ;

private:
	G::Slot::Signal1<bool> m_done_signal ;
	GNet::ExceptionHandler & m_exception_handler ;
	bool m_server_side ;
	GNet::Location m_location ;
	unsigned int m_connection_timeout ;
	unsigned int m_response_timeout ;
	GNet::ClientPtr<SpamClient> m_client ;
	std::string m_text ;
} ;

#endif
