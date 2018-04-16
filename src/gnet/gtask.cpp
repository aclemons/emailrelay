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
	TaskImp( Task & , ExceptionHandler & eh , const G::Executable & ,
		const std::string & exec_error_format , const G::Identity & id ) ;
			// Constructor. Spawns the child processes and starts the
			// associated wait() thread.
			//
			// The GNet::FutureEvent class is used to send the completion
			// message from the wait() thread to the main thread via
			// the event-loop.
			//
			// In a single-threaded build the construction of 'm_thread'
			// sub-object performs a blocking wait() and the completion
			// message is posted to the event-loop before this constructor
			// returns.

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

GNet::TaskImp::TaskImp( Task & task , ExceptionHandler & eh , const G::Executable & commandline ,
	const std::string & exec_error_format , const G::Identity & id ) :
		m_task(task) ,
		m_future_event(*this,eh) ,
		m_process(commandline.exe(),commandline.args(),1,true,true,id,true,127,exec_error_format) ,
		m_thread(TaskImp::wait,this,m_future_event.handle())
{
}

GNet::TaskImp::~TaskImp()
{
	if( m_thread.joinable() )
	{
		m_process.kill() ;
		G_DEBUG( "TaskImp::dtor: waiting for waitfor() thread " << m_thread.get_id() << " to complete" ) ;
		m_thread.join() ;
	}
}

void GNet::TaskImp::wait( TaskImp * This , FutureEvent::handle_type handle )
{
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
	m_thread.join() ;

	int exit_code = m_process.wait().get() ;
	G_DEBUG( "GNet::TaskImp::onFutureEvent: exit code " << exit_code ) ;

	std::string output = m_process.wait().output() ;
	G_DEBUG( "GNet::TaskImp::onFutureEvent: output: [" << G::Str::printable(output) << "]" ) ;

	m_task.done( exit_code , output ) ; // last
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

void GNet::Task::start( const G::Executable & commandline )
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

