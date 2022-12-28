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
/// \file gnullfilter.h
///

#ifndef G_NULL_FILTER_H
#define G_NULL_FILTER_H

#include "gdef.h"
#include "gtimer.h"
#include "gfilter.h"

namespace GFilters
{
	class NullFilter ;
}

//| \class GFilters::NullFilter
/// A Filter class that does nothing.
///
class GFilters::NullFilter : public GSmtp::Filter
{
public:
	NullFilter( GNet::ExceptionSink , Filter::Type ) ;
		///< Constructor.

	NullFilter( GNet::ExceptionSink , Filter::Type , unsigned int exit_code ) ;
		///< Constructor for a processor that behaves like an
		///< executable that always exits with the given
		///< exit code.

private: // overrides
	std::string id() const override ; // GSmtp::Filter
	bool simple() const override ; // GSmtp::Filter
	G::Slot::Signal<int> & doneSignal() override ; // GSmtp::Filter
	void start( const GStore::MessageId & ) override ; // GSmtp::Filter
	void cancel() override ; // GSmtp::Filter
	Result result() const override ; // GSmtp::Filter
	std::string response() const override ; // GSmtp::Filter
	std::string reason() const override ; // GSmtp::Filter
	bool special() const override ; // GSmtp::Filter

public:
	~NullFilter() override = default ;
	NullFilter( const NullFilter & ) = delete ;
	NullFilter( NullFilter && ) = delete ;
	NullFilter & operator=( const NullFilter & ) = delete ;
	NullFilter & operator=( NullFilter && ) = delete ;

private:
	void onTimeout() ;

private:
	G::Slot::Signal<int> m_done_signal ;
	Filter::Exit m_exit ;
	std::string m_id ;
	GNet::Timer<NullFilter> m_timer ;
} ;

#endif
