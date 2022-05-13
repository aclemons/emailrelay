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
/// \file gfilterchain.h
///

#ifndef G_SMTP_FILTER_CHAIN_H
#define G_SMTP_FILTER_CHAIN_H

#include "gdef.h"
#include "gfilter.h"
#include "gfilterfactory.h"
#include "gtimer.h"
#include "gslot.h"
#include "gexceptionsink.h"
#include <memory>
#include <vector>

namespace GSmtp
{
	class FilterChain ;
	class FileStore ;
}

//| \class GSmtp::FilterChain
/// A Filter class that runs an external helper program.
///
class GSmtp::FilterChain : public Filter
{
public:
	FilterChain( GNet::ExceptionSink , FileStore & , FilterFactory & ,
		bool server_side , const std::string & spec , unsigned int timeout ) ;
			///< Constructor.

	~FilterChain() override ;
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
	FilterChain( const FilterChain & ) = delete ;
	FilterChain( FilterChain && ) = delete ;
	FilterChain & operator=( const FilterChain & ) = delete ;
	FilterChain & operator=( FilterChain && ) = delete ;

private:
	void onFilterDone( int ) ;

private:
	FileStore & m_file_store ;
	G::Slot::Signal<int> m_done_signal ;
	bool m_server_side ;
	std::string m_filter_id ;
	std::vector<std::unique_ptr<Filter>> m_filters ;
	std::size_t m_filter_index ;
	Filter * m_filter ;
	bool m_running ;
	MessageId m_message_id ;
} ;

#endif
