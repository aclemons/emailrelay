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
/// \file gexecutablefilter.h
///

#ifndef G_SMTP_EXECUTABLE_FILTER__H
#define G_SMTP_EXECUTABLE_FILTER__H

#include "gdef.h"
#include "gsmtp.h"
#include "gpath.h"
#include "gfilter.h"
#include "geventhandler.h"
#include "gfutureevent.h"
#include "gtask.h"
#include <utility>

namespace GSmtp
{
	class ExecutableFilter ;
}

/// \class GSmtp::ExecutableFilter
/// A Filter class that runs an external helper program.
///
class GSmtp::ExecutableFilter : public Filter, private GNet::TaskCallback
{
public:
	ExecutableFilter( GNet::ExceptionHandler & , bool server_side , const std::string & ) ;
		///< Constructor.

	virtual ~ExecutableFilter() ;
		///< Destructor.

	virtual std::string id() const override ;
		///< Override from from GSmtp::Filter.

	virtual bool simple() const override ;
		///< Override from from GSmtp::Filter.

	virtual G::Slot::Signal1<int> & doneSignal() override ;
		///< Override from from GSmtp::Filter.

	virtual void start( const std::string & path ) override ;
		///< Override from from GSmtp::Filter.

	virtual void cancel() override ;
		///< Override from from GSmtp::Filter.

	virtual bool abandoned() const override ;
		///< Override from from GSmtp::Filter.

	virtual std::string response() const override ;
		///< Override from from GSmtp::Filter.

	virtual std::string reason() const override ;
		///< Override from from GSmtp::Filter.

	virtual bool special() const override ;
		///< Override from from GSmtp::Filter.

private:
	ExecutableFilter( const ExecutableFilter & ) ; // not implemented
	void operator=( const ExecutableFilter & ) ; // not implemented
	std::pair<std::string,std::string> parseOutput( std::string , const std::string & ) const ;
	virtual void onTaskDone( int , const std::string & ) override ; // override from GNet::TaskCallback

private:
	G::Slot::Signal1<int> m_done_signal ;
	bool m_server_side ;
	std::string m_prefix ;
	Exit m_exit ;
	G::Path m_path ;
	std::string m_response ;
	std::string m_reason ;
	GNet::Task m_task ;
} ;

#endif
