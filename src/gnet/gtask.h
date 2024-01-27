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
/// \file gtask.h
///

#ifndef G_NET_TASK_H
#define G_NET_TASK_H

#include "gdef.h"
#include "geventhandler.h"
#include "genvironment.h"
#include "gnewprocess.h"
#include "gexceptionsink.h"
#include "gexception.h"
#include "gidentity.h"
#include "gexecutablecommand.h"
#include <memory>

namespace GNet
{
	class Task ;
	class TaskImp ;
	class TaskCallback ;
}

//| \class GNet::Task
/// A class for running an exectuable in a separate process with an asychronous
/// completion callback.
///
class GNet::Task
{
public:
	G_EXCEPTION( Busy , tx("cannot execute command-line task: still busy from last time") ) ;

	Task( TaskCallback & , ExceptionSink es ,
		const std::string & exec_error_format = {} ,
		const G::Identity & = G::Identity::invalid() ) ;
			///< Constructor for an object that can be start()ed or run().
			///< The two trailing parameters are passed to the G::NewProcess
			///< class.

	~Task() ;
		///< Destructor. Kills the spawned process and waits for it to
		///< terminate, where necessary.

	void start( const G::ExecutableCommand & commandline ) ;
		///< Starts the task by spawning a new process with the given
		///< command-line and also starting a thread to wait for it. The
		///< wait thread signals completion of the child process via the
		///< event loop and the TaskCallback interface. Standard
		///< output goes to the pipe and standard error is discarded.
		///< Throws Busy if still busy from a prior call to start().

	void start( const G::ExecutableCommand & commandline , const G::Environment & env ,
		G::NewProcess::Fd fd_stdin = G::NewProcess::Fd::devnull() ,
		G::NewProcess::Fd fd_stdout = G::NewProcess::Fd::pipe() ,
		G::NewProcess::Fd fd_stderr = G::NewProcess::Fd::devnull() ,
		const G::Path & cd = G::Path() ) ;
			///< Overload with more control over the execution
			///< environment. See also G::NewProcess.

	void stop() ;
		///< Attempts to kill the spawned process. No task-done
		///< callback will be triggered.

	std::pair<int,std::string> run( const G::ExecutableCommand & commandline , const G::Environment & env ,
		G::NewProcess::Fd fd_stdin = G::NewProcess::Fd::devnull() ,
		G::NewProcess::Fd fd_stdout = G::NewProcess::Fd::pipe() ,
		G::NewProcess::Fd fd_stderr = G::NewProcess::Fd::devnull() ,
		const G::Path & cd = G::Path() ) ;
			///< Runs the task synchronously and returns the exit code
			///< and pipe output. Throws if killed. The callback interface
			///< is not used.

public:
	Task( const Task & ) = delete ;
	Task( Task && ) = delete ;
	Task & operator=( const Task & ) = delete ;
	Task & operator=( Task && ) = delete ;

private:
	friend class GNet::TaskImp ;
	void done( int exit_code , const std::string & output ) ;
	void exception( std::exception & ) ;

private:
	std::unique_ptr<TaskImp> m_imp ;
	TaskCallback & m_callback ;
	ExceptionSink m_es ;
	std::string m_exec_error_format ;
	G::Identity m_id ;
	bool m_busy {false} ;
} ;

//| \class GNet::TaskCallback
/// An abstract interface for callbacks from GNet::Task.
///
class GNet::TaskCallback
{
public:
	virtual ~TaskCallback() = default ;
		///< Destructor.

	virtual void onTaskDone( int exit_status , const std::string & pipe_output ) = 0 ;
		///< Callback function to signal task completion.
} ;

#endif
