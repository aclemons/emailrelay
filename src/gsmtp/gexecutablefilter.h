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
/// \file gexecutablefilter.h
///

#ifndef G_SMTP_EXECUTABLE_FILTER_H
#define G_SMTP_EXECUTABLE_FILTER_H

#include "gdef.h"
#include "gpath.h"
#include "gfilter.h"
#include "gfilestore.h"
#include "geventhandler.h"
#include "gfutureevent.h"
#include "gtimer.h"
#include "gtask.h"
#include <utility>

namespace GSmtp
{
	class ExecutableFilter ;
}

//| \class GSmtp::ExecutableFilter
/// A Filter class that runs an external helper program.
///
class GSmtp::ExecutableFilter : public Filter, private GNet::TaskCallback
{
public:
	ExecutableFilter( GNet::ExceptionSink , FileStore & , bool server_side ,
		const std::string & path , unsigned int timeout ) ;
			///< Constructor.

	~ExecutableFilter() override ;
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
	void onTaskDone( int , const std::string & ) override ; // Override from GNet::TaskCallback.

public:
	ExecutableFilter( const ExecutableFilter & ) = delete ;
	ExecutableFilter( ExecutableFilter && ) = delete ;
	ExecutableFilter & operator=( const ExecutableFilter & ) = delete ;
	ExecutableFilter & operator=( ExecutableFilter && ) = delete ;

private:
	std::pair<std::string,std::string> parseOutput( std::string , const std::string & ) const ;
	void onTimeout() ;

private:
	FileStore & m_file_store ;
	G::Slot::Signal<int> m_done_signal ;
	bool m_server_side ;
	std::string m_prefix ;
	Exit m_exit ;
	G::Path m_path ;
	unsigned int m_timeout ;
	GNet::Timer<ExecutableFilter> m_timer ;
	std::string m_response ;
	std::string m_reason ;
	GNet::Task m_task ;
} ;

#endif
