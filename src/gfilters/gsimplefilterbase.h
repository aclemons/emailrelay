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
/// \file gsimplefilterbase.h
///

#ifndef G_SIMPLE_FILTER_BASE_H
#define G_SIMPLE_FILTER_BASE_H

#include "gdef.h"
#include "gfilter.h"
#include "gfilestore.h"
#include "genvelope.h"
#include "gexceptionsink.h"
#include "gslot.h"
#include "gtimer.h"
#include "gstringview.h"

namespace GFilters
{
	class SimpleFilterBase ;
}

//| \class GFilters::SimpleFilterBase
/// A GSmtp::Filter base class for filters that run synchronously.
/// Concrete classes should implement run().
///
class GFilters::SimpleFilterBase : public GSmtp::Filter
{
public:
	SimpleFilterBase( GNet::ExceptionSink , Filter::Type , G::string_view id ) ;
		///< Constructor.

	virtual Result run( const GStore::MessageId & , bool & special_out ,
		GStore::FileStore::State ) = 0 ;
			///< Runs the filter synchronously and returns the result.

	std::string prefix() const ;
		///< Returns a logging prefix derived from Filter::Type and filter id.

private: // overrides
	bool quiet() const final ;
	void start( const GStore::MessageId & ) final ;
	std::string id() const final ;
	G::Slot::Signal<int> & doneSignal() noexcept final ;
	void cancel() final ;
	Result result() const final ;
	std::string response() const final ;
	int responseCode() const final ;
	std::string reason() const final ;
	bool special() const final ;

private:
	void onTimeout() ;

private:
	Filter::Type m_filter_type ;
	std::string m_id ;
	GNet::Timer<SimpleFilterBase> m_timer ;
	G::Slot::Signal<int> m_done_signal ;
	Result m_result {Result::fail} ;
	bool m_special {false} ;
} ;

#endif
