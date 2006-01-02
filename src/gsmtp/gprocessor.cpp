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
// gprocessor.cpp
//

#include "gdef.h"
#include "gsmtp.h"
#include "gprocess.h"
#include "gprocessor.h"
#include "gstr.h"
#include "groot.h"
#include "glog.h"

GSmtp::Processor::Processor( const G::Executable & exe ) :
	m_exe(exe) ,
	m_ok(true) ,
	m_cancelled(false) ,
	m_repoll(false)
{
}

bool GSmtp::Processor::cancelled() const
{
	return m_cancelled ;
}

bool GSmtp::Processor::repoll() const
{
	return m_repoll ;
}

std::string GSmtp::Processor::text( const std::string & default_ ) const
{
	if( m_ok ) return std::string() ;
	return m_text.empty() ? default_ : m_text ;
}

bool GSmtp::Processor::process( const std::string & path )
{
	if( m_exe.exe() == G::Path() )
	{
		m_ok = true ;
		return true ;
	}

	int exit_code = preprocessCore( G::Path(path) ) ;

	// zero, special or failure
	bool is_zero = exit_code == 0 ;
	bool is_special = exit_code >= 100 && exit_code <= 107 ;
	bool is_failure = !is_zero && !is_special ;
	if( is_failure )
	{
		G_WARNING( "GSmtp::Processor::preprocess: pre-processing failed: exit code " << exit_code ) ;
	}

	// set special-repoll and special-cancelled flags
	m_repoll = is_special && ((exit_code-100)&2) != 0 ;
	m_cancelled = is_special && ((exit_code-100)&1) == 0 ;
	if( m_cancelled )
	{
		G_LOG( "GSmtp::Processor: message processing cancelled by preprocessor" ) ;
	}

	// treat special as ok, except for special-cancelled
	m_ok = is_zero || ( is_special && !m_cancelled ) ;
	return m_ok ;
}

int GSmtp::Processor::preprocessCore( const G::Path & path )
{
	G_LOG( "GSmtp::Processor::preprocess: executable \"" << m_exe.exe() << "\": file \"" << path << "\"" ) ;
	G::Strings args( m_exe.args() ) ;
	args.push_back( path.str() ) ;
	std::string raw_output ;
	int exit_code = G::Process::spawn( G::Root::nobody() , m_exe.exe() , args , &raw_output , 
		127 , execErrorHandler ) ;
	m_text = parseOutput( raw_output ) ;
	G_LOG( "GSmtp::Processor::preprocess: exit status " << exit_code << " (\"" << m_text << "\")" ) ;
	return exit_code ;
}

std::string GSmtp::Processor::execErrorHandler( int error )
{
	// (this runs in the fork()ed child process)
	std::ostringstream ss ;
	ss << "<<exec error " << error << ": " << G::Str::lower(G::Process::strerror(error)) << ">>" ;
	return ss.str() ;
}

std::string GSmtp::Processor::parseOutput( std::string s ) const
{
	G_DEBUG( "GSmtp::Processor::parseOutput: in: \"" << G::Str::toPrintableAscii(s) << "\"" ) ;
	const std::string start("<<") ;
	const std::string end(">>") ;
	std::string result ;
	G::Str::replaceAll( s , "\r\n" , "\n" ) ;
	G::Str::replaceAll( s , "\r" , "\n" ) ;
	G::Strings lines ;
	G::Str::splitIntoFields( s , lines , "\n" ) ;
	for( G::Strings::iterator p = lines.begin() ; p != lines.end() ; ++p )
	{
		std::string line = *p ;
		size_t pos_start = line.find(start) ;
		size_t pos_end = line.find(end) ;
		if( pos_start == 0U && pos_end != std::string::npos )
		{
			result = G::Str::toPrintableAscii(line.substr(start.length(),pos_end-start.length())) ;
		}
	}
	G_DEBUG( "GSmtp::Processor::parseOutput: in: \"" << G::Str::toPrintableAscii(result) << "\"" ) ;
	return result ;
}

G::Signal1<bool> & GSmtp::Processor::doneSignal()
{
	return m_done_signal ;
}

void GSmtp::Processor::abort()
{
	// no-op -- not asynchronous
}

void GSmtp::Processor::start( const std::string & message_file )
{
	// not asynchronous yet -- process() syncronously and emit the done signal
	bool ok = process( message_file ) ;
	m_done_signal.emit( ok ) ;
}


