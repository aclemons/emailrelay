//
// Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gtask.h
///

#ifndef G_NET_TASK__H
#define G_NET_TASK__H

#include "gdef.h"
#include "geventhandler.h"
#include "gexceptionsink.h"
#include "gidentity.h"
#include "gexecutablecommand.h"
#include <memory>

namespace GNet
{
	class Task ;
	class TaskImp ;
	class TaskCallback ;
}

/// \class GNet::Task
/// A class for running an exectuable in a separate process with an asychronous
/// completion callback.
///
class GNet::Task
{
public:
	G_EXCEPTION( Busy , "cannot execute command-line task: still busy from last time" ) ;

	Task( TaskCallback & , ExceptionSink es ,
		const std::string & exec_error_format = std::string() ,
		const G::Identity & = G::Identity::invalid() ) ;
			///< Constructor for a start()able object. The two trailing
			///< parameters are passed to the G::NewProcess class.

	~Task() ;
		///< Destructor. Kills the spawned process and waits for it to
		///< terminate, where necessary.

	void start( const G::ExecutableCommand & commandline ) ;
		///< Starts the task by spawning a new process with the given
		///< command-line and also starting a thread to wait for it. The
		///< wait thread signals completion of the child process via the
		///< event loop and the TaskCallback interface. Throws Busy if
		///< still busy from a prior call to start().

	void stop() ;
		///< Attempts to kill the spawned process and waits for it
		///< to terminate. No task-done callback will be triggered.

private:
	Task( const Task & ) g__eq_delete ;
	void operator=( const Task & ) g__eq_delete ;
	friend class GNet::TaskImp ;
	void done( int exit_code , std::string output ) ;
	void exception( std::exception & ) ;

private:
	unique_ptr<TaskImp> m_imp ;
	TaskCallback & m_callback ;
	ExceptionSink m_es ;
	std::string m_exec_error_format ;
	G::Identity m_id ;
	bool m_busy ;
} ;

/// \class GNet::TaskCallback
/// An abstract interface for callbacks from GNet::Task.
///
class GNet::TaskCallback
{
public:
	virtual ~TaskCallback() ;
		///< Destructor.

	virtual void onTaskDone( int exit_status , const std::string & output ) = 0 ;
		///< Callback function to signal task completion.
} ;

#endif
