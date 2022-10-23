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
#include "gfactoryparser.h"
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
/// A Filter class that runs a sequence of sub-filters. The sub-filters are run
/// in sequence only as long as they return success.
///
class GSmtp::FilterChain : public Filter
{
public:
	FilterChain( GNet::ExceptionSink , FilterFactory & , bool server_side ,
		const FactoryParser::Result & spec , unsigned int timeout ,
		const std::string & log_prefix ) ;
			///< Constructor.

	~FilterChain() override ;
		///< Destructor.

public:
	FilterChain( const FilterChain & ) = delete ;
	FilterChain( FilterChain && ) = delete ;
	FilterChain & operator=( const FilterChain & ) = delete ;
	FilterChain & operator=( FilterChain && ) = delete ;

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

private:
	void add( GNet::ExceptionSink , FilterFactory & , bool , const FactoryParser::Result & , unsigned int , const std::string & ) ;
	void onFilterDone( int ) ;

private:
	G::Slot::Signal<int> m_done_signal ;
	std::string m_filter_id ;
	std::vector<std::unique_ptr<Filter>> m_filters ;
	std::size_t m_filter_index ;
	Filter * m_filter ;
	bool m_running ;
	MessageId m_message_id ;
} ;

#endif