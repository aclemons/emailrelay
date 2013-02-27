//
// Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gexecutableprocessor.cpp
//

#include "gdef.h"
#include "gsmtp.h"
#include "gprocess.h"
#include "gnewprocess.h"
#include "gexecutableprocessor.h"
#include "gstr.h"
#include "groot.h"
#include "glog.h"

GSmtp::ExecutableProcessor::ExecutableProcessor( const G::Executable & exe ) :
	m_exe(exe) ,
	m_ok(true) ,
	m_cancelled(false) ,
	m_repoll(false)
{
}

GSmtp::ExecutableProcessor::~ExecutableProcessor()
{
}

bool GSmtp::ExecutableProcessor::cancelled() const
{
	return m_cancelled ;
}

bool GSmtp::ExecutableProcessor::repoll() const
{
	return m_repoll ;
}

std::string GSmtp::ExecutableProcessor::text() const
{
	if( m_ok ) return std::string() ;
	const std::string default_ = "pre-processing failed" ;
	return m_text.empty() ? default_ : m_text ;
}

bool GSmtp::ExecutableProcessor::process( const std::string & path )
{
	int exit_code = preprocessCore( G::Path(path) ) ;

	// zero, special or failure
	bool is_zero = exit_code == 0 ;
	bool is_special = exit_code >= 100 && exit_code <= 107 ;
	bool is_failure = !is_zero && !is_special ;
	if( is_failure )
	{
		G_WARNING( "GSmtp::ExecutableProcessor::preprocess: pre-processing failed: exit code " << exit_code ) ;
	}

	// set special-repoll and special-cancelled flags
	m_repoll = is_special && ((exit_code-100)&2) != 0 ;
	m_cancelled = is_special && ((exit_code-100)&1) == 0 ;

	// treat special as ok, except for special-cancelled
	m_ok = is_zero || ( is_special && !m_cancelled ) ;
	return m_ok ;
}

int GSmtp::ExecutableProcessor::preprocessCore( const G::Path & path )
{
	G_LOG( "GSmtp::ExecutableProcessor::preprocess: running \"" << m_exe.displayString() << " " << path << "\"" ) ;

	// add the path of the content file as a trailing command-line parameter
	G::Strings args( m_exe.args() ) ;
	args.push_back( path.str() ) ;

	// run the program
	std::string raw_output ;
	int exit_code = G::NewProcess::spawn( G::Root::nobody() , m_exe.exe() , args , 
		&raw_output , 127 , execErrorHandler ) ;

	// search the output for diagnostics
	m_text = parseOutput( raw_output ) ;
	G_LOG( "GSmtp::ExecutableProcessor::preprocess: exit status " << exit_code << " (\"" << m_text << "\")" ) ;

	return exit_code ;
}

std::string GSmtp::ExecutableProcessor::execErrorHandler( int error )
{
	// (this runs in the fork()ed child process)
	std::ostringstream ss ;
	ss << "<<exec error " << error << ": " << G::Str::lower(G::Process::strerror(error)) << ">>" ;
	return ss.str() ;
}

std::string GSmtp::ExecutableProcessor::parseOutput( std::string s ) const
{
	G_DEBUG( "GSmtp::ExecutableProcessor::parseOutput: in: \"" << G::Str::printable(s) << "\"" ) ;

	const std::string start_1("<<") ;
	const std::string end_1(">>") ;
	const std::string start_2("[[") ;
	const std::string end_2("]]") ;

	std::string result ;
	while( G::Str::replaceAll( s , "\r\n" , "\n" ) ) ;
	G::Str::replaceAll( s , "\r" , "\n" ) ;
	G::Strings lines ;
	G::Str::splitIntoFields( s , lines , "\n" ) ;
	for( G::Strings::iterator p = lines.begin() ; p != lines.end() ; ++p )
	{
		std::string line = *p ;
		size_t pos_start = line.find(start_1) ;
		size_t pos_end = line.find(end_1) ;
		if( pos_start != 0U )
		{
			pos_start = line.find(start_2) ;
			pos_end = line.find(end_2) ;
		}
		if( pos_start == 0U && pos_end != std::string::npos )
		{
			result = G::Str::printable(line.substr(2U,pos_end-2U)) ;
			break ;
		}
	}
	G_DEBUG( "GSmtp::ExecutableProcessor::parseOutput: out: \"" << G::Str::printable(result) << "\"" ) ;
	return result ;
}

G::Signal1<bool> & GSmtp::ExecutableProcessor::doneSignal()
{
	return m_done_signal ;
}

void GSmtp::ExecutableProcessor::abort()
{
	// no-op -- not asynchronous
}

void GSmtp::ExecutableProcessor::start( const std::string & message_file )
{
	// not asynchronous -- process() synchronously and emit the done signal
	bool ok = process( message_file ) ;
	m_done_signal.emit( ok ) ;
}

/// \file gexecutableprocessor.cpp
