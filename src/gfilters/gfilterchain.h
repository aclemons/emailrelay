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
/// \file gfilterchain.h
///

#ifndef G_FILTER_CHAIN_H
#define G_FILTER_CHAIN_H

#include "gdef.h"
#include "gfilter.h"
#include "gfilterfactory.h"
#include "gtimer.h"
#include "gslot.h"
#include "gexceptionsink.h"
#include <memory>
#include <vector>

namespace GFilters
{
	class FilterChain ;
}

//| \class GFilters::FilterChain
/// A Filter class that runs a sequence of sub-filters. The sub-filters are run
/// in sequence only as long as they return success.
///
class GFilters::FilterChain : public GSmtp::Filter
{
public:
	FilterChain( GNet::ExceptionSink , GSmtp::FilterFactoryBase & , Filter::Type ,
		const Filter::Config & , const GSmtp::FilterFactoryBase::Spec & spec ) ;
			///< Constructor.

	~FilterChain() override ;
		///< Destructor.

public:
	FilterChain( const FilterChain & ) = delete ;
	FilterChain( FilterChain && ) = delete ;
	FilterChain & operator=( const FilterChain & ) = delete ;
	FilterChain & operator=( FilterChain && ) = delete ;

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

private:
	void add( GNet::ExceptionSink , GSmtp::FilterFactoryBase & , Filter::Type ,
		const Filter::Config & , const GSmtp::FilterFactoryBase::Spec & ) ;
	void onFilterDone( int ) ;

private:
	G::Slot::Signal<int> m_done_signal ;
	std::string m_filter_id ;
	std::vector<std::unique_ptr<GSmtp::Filter>> m_filters ;
	std::size_t m_filter_index ;
	GSmtp::Filter * m_filter ;
	bool m_running ;
	GStore::MessageId m_message_id ;
} ;

#endif
