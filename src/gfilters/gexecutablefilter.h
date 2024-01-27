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
/// \file gexecutablefilter.h
///

#ifndef G_EXECUTABLE_FILTER_H
#define G_EXECUTABLE_FILTER_H

#include "gdef.h"
#include "gpath.h"
#include "gfilter.h"
#include "gfilestore.h"
#include "geventhandler.h"
#include "gfutureevent.h"
#include "gtimer.h"
#include "gtask.h"
#include <utility>
#include <tuple>

namespace GFilters
{
	class ExecutableFilter ;
}

//| \class GFilters::ExecutableFilter
/// A Filter class that runs an external helper program.
///
class GFilters::ExecutableFilter : public GSmtp::Filter, private GNet::TaskCallback
{
public:
	ExecutableFilter( GNet::ExceptionSink , GStore::FileStore & , Filter::Type ,
		const Filter::Config & , const std::string & path ) ;
			///< Constructor.

	~ExecutableFilter() override ;
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
	void onTaskDone( int , const std::string & ) override ; // GNet::TaskCallback

public:
	ExecutableFilter( const ExecutableFilter & ) = delete ;
	ExecutableFilter( ExecutableFilter && ) = delete ;
	ExecutableFilter & operator=( const ExecutableFilter & ) = delete ;
	ExecutableFilter & operator=( ExecutableFilter && ) = delete ;

private:
	std::tuple<std::string,int,std::string> parseOutput( std::string , const std::string & ) const ;
	void onTimeout() ;
	std::string prefix() const ;

private:
	GStore::FileStore & m_file_store ;
	G::Slot::Signal<int> m_done_signal ;
	Filter::Type m_filter_type ;
	Exit m_exit ;
	G::Path m_path ;
	unsigned int m_timeout ;
	GNet::Timer<ExecutableFilter> m_timer ;
	std::string m_response ;
	int m_response_code {0} ;
	std::string m_reason ;
	GNet::Task m_task ;
} ;

#endif
