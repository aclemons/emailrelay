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

	bool process( const G::Path & message_file ) ;
		// Runs the processor. Returns false on error.
		// Does nothing, returning true if the exe
		// path is empty.
		//
		// If the program's standard output contains
		// a line like "<<text>>" then the text string 
		// is returned by text().

	std::string text( const std::string & default_ = std::string() ) const ;
		// Returns text output by the processor program,
		// or the default text if no diagnostic output was 
		// received.

	bool cancelled() const ;
		// Returns true if further message processesing
		// is to be cancelled.

	bool repoll() const ;
		// Returns true if the message store should be
		// repolled immediately.

private:
	Processor( const Processor & ) ; // not implemented
	void operator=( const Processor & ) ; // not implemented
	int preprocessCore( const G::Path & ) ;
	std::string parseOutput( std::string ) const ;

private:
	G::Executable m_exe ;
	std::string m_text ;
	bool m_cancelled ;
	bool m_repoll ;
} ;

#endif
