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
/// \file gtask.cpp
///

#include "gdef.h"
#include "gtask.h"
#include "gfutureevent.h"
#include "gnewprocess.h"
#include "gcleanup.h"
#include "gtimer.h"
#include "gstr.h"
#include "gtest.h"
#include "gassert.h"
#include "glog.h"

//| \class GNet::TaskImp
/// A private implementation class used by GNet::Task.
///
class GNet::TaskImp : private FutureEventHandler
{
public:
	TaskImp( Task & , ExceptionSink es , bool sync ,
		const G::ExecutableCommand & , const G::Environment & env ,
		G::NewProcess::Fd fd_stdin , G::NewProcess::Fd fd_stdout , G::NewProcess::Fd fd_stderr ,
		const G::Path & cd , const std::string & exec_error_format , const G::Identity & id ) ;
			// Constructor. Spawns the child processes.
			//
			// The GNet::FutureEvent class is used to send the completion
			// message from the waitpid(2) thread to the main thread via
			// the event-loop.
			//
			// In a single-threaded build, or if multi-threading is broken,
			// this constructor runs the task, waits for it to complete
			// and posts the completion message to the event-loop
			// before this constructor returns.

	~TaskImp() override ;
		// Destructor.

	void start() ;
		// Starts a waitpid() thread asynchronously that emits a
		// FutureEvent when done.

	std::pair<int,std::string> wait() ;
		// Runs waitpid() synchronously.
		// Precondition: ctor 'sync'

	bool zombify() ;
		// Kills the task process and makes this object a zombie that lives
		// only on the timer-list, or returns false.

private: // overrides
	void onFutureEvent() override ; // Override from GNet::FutureEventHandler.

public:
	TaskImp( const TaskImp & ) = delete ;
	TaskImp( TaskImp && ) = delete ;
	TaskImp & operator=( const TaskImp & ) = delete ;
	TaskImp & operator=( TaskImp && ) = delete ;

private:
	void onTimeout() ;
	static void waitThread( TaskImp * , HANDLE ) ; // thread function

private:
	Task * m_task ;
	FutureEvent m_future_event ;
	Timer<TaskImp> m_timer ;
	bool m_logged {false} ;
	G::NewProcess m_process ;
	G::threading::thread_type m_thread ;
	static std::size_t m_zcount ;
} ;

std::size_t GNet::TaskImp::m_zcount = 0U ;

// ==

GNet::TaskImp::TaskImp( Task & task , ExceptionSink es , bool sync ,
	const G::ExecutableCommand & commandline , const G::Environment & env ,
	G::NewProcess::Fd fd_stdin , G::NewProcess::Fd fd_stdout , G::NewProcess::Fd fd_stderr ,
	const G::Path & cd , const std::string & exec_error_format ,
	const G::Identity & id ) :
		m_task(&task) ,
		m_future_event(*this,es) ,
		m_timer(*this,&TaskImp::onTimeout,es) ,
		m_process( commandline.exe() , commandline.args() ,
			G::NewProcess::Config()
				.set_env(env)
				.set_stdin(fd_stdin)
				.set_stdout(fd_stdout)
				.set_stderr(fd_stderr)
				.set_cd(cd)
				.set_strict_exe(true)
				.set_run_as(id)
				.set_strict_id(true)
				.set_exec_error_exit(127)
				.set_exec_error_format(exec_error_format) )
{
	if( sync )
	{
		// no thread -- caller will call synchronous wait() method
	}
	else if( !G::threading::works() )
	{
		if( G::threading::using_std_thread )
			G_WARNING_ONCE( "GNet::TaskImp::TaskImp: multi-threading disabled: running tasks synchronously" ) ;
		waitThread( this , m_future_event.handle() ) ;
	}
	else
	{
		G_ASSERT( G::threading::using_std_thread ) ;
		G::Cleanup::Block block_signals ;
		m_thread = G::threading::thread_type( TaskImp::waitThread , this , m_future_event.handle() ) ;
	}
}

GNet::TaskImp::~TaskImp()
{
	try
	{
		// (should be already join()ed)
		if( m_thread.joinable() )
			m_process.kill( true ) ;
		if( m_thread.joinable() )
			m_thread.join() ;
	}
	catch(...)
	{
	}
}

bool GNet::TaskImp::zombify()
{
	m_task = nullptr ;
	if( m_thread.joinable() )
	{
		if( !G::Test::enabled("task-no-kill") )
			m_process.kill( true ) ;

		m_zcount++ ;
		m_timer.startTimer( 1U ) ;

		const std::size_t threshold = 30U ;
		if( m_zcount == threshold )
			G_WARNING_ONCE( "GNet::Task::dtor: large number of threads waiting for processes to finish" ) ;

		return true ;
	}
	else
	{
		return false ;
	}
}

void GNet::TaskImp::onTimeout()
{
	if( m_thread.joinable() )
	{
		if( !m_logged )
			G_LOG( "TaskImp::dtor: waiting for killed process to terminate: pid " << m_process.id() ) ;
		m_logged = true ;
		m_timer.startTimer( 1U ) ;
	}
	else
	{
		if( m_logged )
			G_LOG( "TaskImp::dtor: killed process has terminated: pid " << m_process.id() ) ;
		delete this ;
		m_zcount-- ;
	}
}

std::pair<int,std::string> GNet::TaskImp::wait()
{
	m_process.waitable().wait() ;
	int exit_code = m_process.waitable().get() ;
	return { exit_code , m_process.waitable().output() } ;
}

void GNet::TaskImp::waitThread( TaskImp * This , HANDLE handle )
{
	// worker-thread -- keep it simple
	try
	{
		This->m_process.waitable().wait() ;
		FutureEvent::send( handle ) ;
	}
	catch(...) // worker thread outer function
	{
		FutureEvent::send( handle ) ; // noexcept
	}
}

void GNet::TaskImp::onFutureEvent()
{
	G_DEBUG( "GNet::TaskImp::onFutureEvent: future event" ) ;
	if( m_thread.joinable() )
		m_thread.join() ;

	int exit_code = m_process.waitable().get( std::nothrow ) ;
	G_DEBUG( "GNet::TaskImp::onFutureEvent: exit code " << exit_code ) ;

	std::string pipe_output = m_process.waitable().output() ;
	G_LOG_MORE( "GNet::TaskImp::onFutureEvent: executable output: [" << G::Str::printable(pipe_output) << "]" ) ;

	if( m_task )
		m_task->done( exit_code , pipe_output ) ; // last
}

// ==

GNet::Task::Task( TaskCallback & callback , ExceptionSink es ,
	const std::string & exec_error_format , const G::Identity & id ) :
		m_callback(callback) ,
		m_es(es) ,
		m_exec_error_format(exec_error_format) ,
		m_id(id)
{
}

GNet::Task::~Task()
{
	try
	{
		stop() ;
	}
	catch(...) // dtor
	{
	}
}

void GNet::Task::stop()
{
	// kill the process and release the imp to an independent life
	// on the timer-list while the thread finishes up
	if( m_imp && m_imp->zombify() )
		GDEF_IGNORE_RETURN m_imp.release() ;
	m_busy = false ;
}

#ifndef G_LIB_SMALL
std::pair<int,std::string> GNet::Task::run( const G::ExecutableCommand & commandline ,
	const G::Environment & env ,
	G::NewProcess::Fd fd_stdin ,
	G::NewProcess::Fd fd_stdout ,
	G::NewProcess::Fd fd_stderr ,
	const G::Path & cd )
{
	G_ASSERT( !m_busy ) ;
	m_imp = std::make_unique<TaskImp>( *this , m_es , true , commandline ,
		env , fd_stdin , fd_stdout , fd_stderr , cd ,
		m_exec_error_format , m_id ) ;
	return m_imp->wait() ;
}
#endif

void GNet::Task::start( const G::ExecutableCommand & commandline )
{
	start( commandline , G::Environment::minimal() ,
		G::NewProcess::Fd::devnull() ,
		G::NewProcess::Fd::pipe() ,
		G::NewProcess::Fd::devnull() ,
		G::Path() ) ;
}

void GNet::Task::start( const G::ExecutableCommand & commandline ,
	const G::Environment & env ,
	G::NewProcess::Fd fd_stdin ,
	G::NewProcess::Fd fd_stdout ,
	G::NewProcess::Fd fd_stderr ,
	const G::Path & cd )
{
	if( m_busy )
		throw Busy() ;

	m_busy = true ;
	m_imp = std::make_unique<TaskImp>( *this , m_es , false , commandline ,
		env , fd_stdin , fd_stdout , fd_stderr , cd ,
		m_exec_error_format , m_id ) ;
}

void GNet::Task::done( int exit_code , const std::string & output )
{
	m_busy = false ;
	m_callback.onTaskDone( exit_code , output ) ;
}

