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
//
// gtask.cpp
//

#include "gdef.h"
#include "gtask.h"
#include "gfutureevent.h"
#include "gnewprocess.h"
#include "gstr.h"
#include "glog.h"

/// \class GNet::TaskImp
/// A private implementation class used by GNet::Task.
///
class GNet::TaskImp : private FutureEventHandler
{
public:
	TaskImp( Task & , ExceptionHandler & eh , const G::ExecutableCommand & ,
		const std::string & exec_error_format , const G::Identity & id ) ;
			// Constructor. Spawns the child processes and starts the
			// associated wait() thread.
			//
			// The GNet::FutureEvent class is used to send the completion
			// message from the wait() thread to the main thread via
			// the event-loop.
			//
			// In a single-threaded build, or if multi-threading is broken,
			// this contructor runs the task, waits for it to complete
			// and posts the completion message to the event-loop
			// before this constructor returns.

	static std::pair<int,std::string> run( const G::ExecutableCommand & ,
		const std::string & exec_error_format , const G::Identity & id ) ;
			// Runs the task synchronously.

	virtual ~TaskImp() ;
		// Destructor.

	void kill() ;
		// Kills the task.

private:
	TaskImp( const TaskImp & ) ;
	void operator=( const TaskImp & ) ;
	virtual void onFutureEvent() override ; // GNet::FutureEventHandler
	static void wait( TaskImp * , FutureEvent::handle_type ) ; // thread function

private:
	Task & m_task ;
	FutureEvent m_future_event ;
	G::NewProcess m_process ;
	G::threading::thread_type m_thread ;
} ;

// ==

GNet::TaskImp::TaskImp( Task & task , ExceptionHandler & eh , const G::ExecutableCommand & commandline ,
	const std::string & exec_error_format , const G::Identity & id ) :
		m_task(task) ,
		m_future_event(*this,eh) ,
		m_process(commandline.exe(),commandline.args(),1,true,true,id,true,127,exec_error_format)
{
	if( ! G::threading::works() ) // grr
	{
		if( G::threading::using_std_thread )
			G_WARNING_ONCE( "GNet::TaskImp::TaskImp: multi-threading disabled: running tasks synchronously" ) ;
		wait( this , m_future_event.handle() ) ;
	}
	else
	{
		m_thread = G::threading::thread_type( TaskImp::wait , this , m_future_event.handle() ) ;
	}
}

GNet::TaskImp::~TaskImp()
{
	if( m_thread.joinable() )
	{
		m_process.kill() ;
		G_WARNING( "TaskImp::dtor: blocked waiting for process " << m_process.id() << " to terminate" ) ;
		m_thread.join() ;
		G_LOG( "TaskImp::dtor: process " << m_process.id() << " terminated" ) ;
	}
}

void GNet::TaskImp::wait( TaskImp * This , FutureEvent::handle_type handle )
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
	try
	{
		G_DEBUG( "GNet::TaskImp::onFutureEvent: future event" ) ;
		if( G::threading::works() )
		{
			m_thread.join() ;
			G_DEBUG( "GNet::TaskImp::onFutureEvent: thread joined" ) ;
		}

		int exit_code = m_process.wait().get() ;
		G_DEBUG( "GNet::TaskImp::onFutureEvent: exit code " << exit_code ) ;

		std::string output = m_process.wait().output() ;
		G_DEBUG( "GNet::TaskImp::onFutureEvent: output: [" << G::Str::printable(output) << "]" ) ;

		m_task.done( exit_code , output ) ; // last
	}
	catch( std::exception & e )
	{
		throw Task::Failed( e.what() ) ;
	}
}

void GNet::TaskImp::kill()
{
	m_process.kill() ;
}

// ==

GNet::Task::Task( TaskCallback & callback , ExceptionHandler & eh ,
	const std::string & exec_error_format , const G::Identity & id ) :
		m_callback(callback) ,
		m_eh(eh) ,
		m_exec_error_format(exec_error_format) ,
		m_id(id) ,
		m_busy(false)
{
}

GNet::Task::~Task()
{
}

void GNet::Task::start( const G::ExecutableCommand & commandline )
{
	if( m_busy )
		throw Busy() ;

	m_busy = true ;
	m_imp.reset( new TaskImp( *this , m_eh , commandline , m_exec_error_format , m_id ) ) ;
}

void GNet::Task::stop()
{
	// best effort - may block
	m_busy = false ;
	m_imp.reset() ;
}

void GNet::Task::done( int exit_code , std::string output )
{
	m_busy = false ;
	m_callback.onTaskDone( exit_code , output ) ;
}

// ==

GNet::TaskCallback::~TaskCallback()
{
}

