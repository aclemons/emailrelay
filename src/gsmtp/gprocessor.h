//
// Copyright (C) 2001-2004 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gprocessor.h
//

#ifndef G_SMTP_PROCESSOR_H
#define G_SMTP_PROCESSOR_H

#include "gdef.h"
#include "gsmtp.h"
#include "gexe.h"
#include "gslot.h"

namespace GSmtp
{
	class Processor ;
}

// Class: GSmtp::Processor
// Description: Processes message files using an external preprocessor
// program.
//
class GSmtp::Processor 
{
public:
	explicit Processor( const G::Executable & preprocessor ) ;
		// Constructor.

	G::Signal1<bool> & doneSignal() ;
		// Returns a signal which is raised once start() has
		// completed. The signal parameter is equivalent
		// to the process() return value.

	void start( const std::string & path ) ;
		// Starts the processor asynchronously.
		// Any previous, incomplete processing 
		// is abort()ed. Asynchronous completion
		// is indicated by the doneSignal().

	void abort() ;
		// Aborts any incomplete processing.

	std::string text( const std::string & default_ = std::string() ) const ;
		// Returns the empty string if process() returned 
		// true, or the "<<text>>" output by the processor
		// program. If process() returned false and
		// there was no text from the processor then
		// the given default string is returned.

	bool cancelled() const ;
		// Returns true if the exit code indicated that 
		// further message processesing is to be cancelled.
		// (If cancelled() then process() returns false.)

	bool repoll() const ;
		// Returns true if the exit code indicated that 
		// the message store should be repolled immediately.
		// (This indicator is independent of cancelled().)

private:
	Processor( const Processor & ) ; // not implemented
	void operator=( const Processor & ) ; // not implemented
	int preprocessCore( const G::Path & ) ;
	std::string parseOutput( std::string ) const ;
	bool process( const std::string & path ) ;
	static std::string execErrorHandler( int error ) ;

private:
	G::Signal1<bool> m_done_signal ;
	G::Executable m_exe ;
	std::string m_text ;
	bool m_ok ;
	bool m_cancelled ;
	bool m_repoll ;
} ;

#endif
