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
/// \file demofilter.h
///

#ifndef MAIN_DEMO_FILTER_H
#define MAIN_DEMO_FILTER_H

#include "gdef.h"
#include "gfilter.h"
#include "gfilestore.h"
#include "gexceptionsink.h"
#include "gtimer.h"
#include "run.h"

namespace Main
{
	class DemoFilter ;
}

//| \class Main::DemoFilter
/// A concrete Filter class that does nothing useful.
///
class Main::DemoFilter : public GSmtp::Filter
{
public:
	DemoFilter( GNet::ExceptionSink es , Main::Run & , unsigned int unit_id ,
		GStore::FileStore & , const std::string & ) ;
			///< Constructor.

	~DemoFilter() override ;
		///< Destructor.

private: // overrided
	std::string id() const override ;
	bool simple() const override ;
	void start( const GStore::MessageId & ) override ;
	G::Slot::Signal<int> & doneSignal() override ;
	void cancel() override ;
	bool abandoned() const override ;
	std::string response() const override ;
	std::string reason() const override ;
	bool special() const override ;

private:
	void onTimeout() ;

private:
	GStore::FileStore & m_store ;
	std::string m_spec ;
	GNet::Timer<DemoFilter> m_timer ;
	G::Slot::Signal<int> m_done_signal ;
} ;

#endif
