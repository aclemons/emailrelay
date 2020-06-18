//
// Copyright (C) 2001-2020 Graeme Walker <graeme_walker@users.sourceforge.net>
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
//
// gtask.cpp
//

#include "gdef.h"
#include "gtask.h"
#include "gfutureevent.h"
#include "gnewprocess.h"
#include "gstr.h"
#include "gassert.h"
#include "glog.h"

/// \class GNet::TaskImp
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
			// this contructor runs the task, waits for it to complete
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

	void kill() ;
		// Kills the task.

private: // overrides
	void onFutureEvent() override ; // Override from GNet::FutureEventHandler.

public:
	TaskImp( const TaskImp & ) = delete ;
	TaskImp( TaskImp && ) = delete ;
	void operator=( const TaskImp & ) = delete ;
	void operator=( TaskImp && ) = delete ;

private:
	static void waitThread( TaskImp * , FutureEvent::handle_type ) ; // thread function

private:
	Task & m_task ;
	FutureEvent m_future_event ;
	G::NewProcess m_process ;
	G::threading::thread_type m_thread ;
} ;

// ==

GNet::TaskImp::TaskImp( Task & task , ExceptionSink es , bool sync ,
	const G::ExecutableCommand & commandline , const G::Environment & env ,
	G::NewProcess::Fd fd_stdin , G::NewProcess::Fd fd_stdout , G::NewProcess::Fd fd_stderr ,
	const G::Path & cd , const std::string & exec_error_format ,
	const G::Identity & id ) :
		m_task(task) ,
		m_future_event(*this,es) ,
		m_process(commandline.exe(),commandline.args(),
			env,
			fd_stdin ,
			fd_stdout ,
			fd_stderr ,
			cd ,
			true , // strict path
			id ,
			true, // strict id
			127 , // exec error exit
			exec_error_format )
{
	if( sync )
	{
	}
	else if( !G::threading::works() )
	{
		if( G::threading::using_std_thread )
			G_WARNING_ONCE( "GNet::TaskImp::TaskImp: multi-threading disabled: running tasks synchronously" ) ;
		waitThread( this , m_future_event.handle() ) ;
	}
	else
	{
		m_thread = G::threading::thread_type( TaskImp::waitThread , this , m_future_event.handle() ) ;
	}
}

GNet::TaskImp::~TaskImp()
{
	if( m_thread.joinable() )
	{
		m_process.kill( /*yield=*/true ) ;
	}
	if( m_thread.joinable() )
	{
		G_LOG_S( "TaskImp::dtor: waiting for killed process to terminate: pid " << m_process.id() ) ;
		m_thread.join() ; // blocks the main event-loop thread
		G_LOG( "TaskImp::dtor: killed process has terminated: pid " << m_process.id() ) ;
	}
}

std::pair<int,std::string> GNet::TaskImp::wait()
{
	m_process.wait().run() ;
	return std::make_pair( m_process.wait().get() , m_process.wait().output() ) ;
}

void GNet::TaskImp::waitThread( TaskImp * This , FutureEvent::handle_type handle )
{
	// worker-thread -- keep it simple
	try
	{
		This->m_process.wait().run() ;
		FutureEvent::send( handle ) ;
	}
	catch(...) // worker thread outer function
	{
		FutureEvent::send( handle ) ; // nothrow
	}
}

void GNet::TaskImp::onFutureEvent()
{
	G_DEBUG( "GNet::TaskImp::onFutureEvent: future event" ) ;
	if( G::threading::works() )
	{
		m_thread.join() ;
		G_DEBUG( "GNet::TaskImp::onFutureEvent: thread joined" ) ;
	}

	int exit_code = m_process.wait().get() ;
	G_DEBUG( "GNet::TaskImp::onFutureEvent: exit code " << exit_code ) ;

	std::string pipe_output = m_process.wait().output() ;
	G_DEBUG( "GNet::TaskImp::onFutureEvent: output: [" << G::Str::printable(pipe_output) << "]" ) ;

	m_task.done( exit_code , pipe_output ) ; // last
}

void GNet::TaskImp::kill()
{
	m_process.kill() ;
}

// ==

GNet::Task::Task( TaskCallback & callback , ExceptionSink es ,
	const std::string & exec_error_format , const G::Identity & id ) :
		m_callback(callback) ,
		m_es(es) ,
		m_exec_error_format(exec_error_format) ,
		m_id(id) ,
		m_busy(false)
{
}

GNet::Task::~Task()
= default;

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

void GNet::Task::stop()
{
	// best effort - may block
	m_busy = false ;
	m_imp.reset() ;
}

void GNet::Task::done( int exit_code , const std::string & output )
{
	m_busy = false ;
	m_callback.onTaskDone( exit_code , output ) ;
}

