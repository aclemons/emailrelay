//
// Copyright (C) 2001-2006 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later
// version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
// ===
//
// thread.h
//

#ifndef THREAD_H__
#define THREAD_H__

#include "qt.h"
#include "gstr.h"
#include "gpath.h"
#include <stdexcept>

// Class: Thread
// Description: A separate thread is used to spin off a process 
// and reads from its pipe. When the thread gets text back it adds 
// it to the text string (inside a mutex lock) and emits a queued 
// "change" signal. When the spawned process exits the thread
// emits  a queued "done" signal.
//
class Thread : public QThread 
{Q_OBJECT
public:
	Thread( G::Path , const G::Strings & ) ;
		// Constructor.

	~Thread() ;
		// Destructor.

	QString text() const ;
		// Returns the accumulated text.

signals:
	void changeSignal() ;
	void doneSignal( int rc ) ;

protected:
	void run() ;

private:
	Thread( const Thread & ) ;
	void operator=( const Thread & ) ;

private:
	mutable QMutex m_mutex ;
	G::Path m_tool ;
	G::Strings m_args ;
	int m_rc ;
	QString m_text ;
} ;

#endif
