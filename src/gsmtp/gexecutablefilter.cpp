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
// gexecutablefilter.cpp
//

#include "gdef.h"
#include "gsmtp.h"
#include "gprocess.h"
#include "gnewprocess.h"
#include "gexecutable.h"
#include "gexecutablefilter.h"
#include "gstr.h"
#include "groot.h"
#include "glog.h"

GSmtp::ExecutableFilter::ExecutableFilter( GNet::ExceptionHandler & eh ,
	bool server_side , const std::string & path ) :
		m_server_side(server_side) ,
		m_path(path) ,
		m_ok(true) ,
		m_special_cancelled(false) ,
		m_special_other(false) ,
		m_task(*this,eh,"<<exec error: __strerror__>>",G::Root::nobody())
{
}

GSmtp::ExecutableFilter::~ExecutableFilter()
{
}

bool GSmtp::ExecutableFilter::simple() const
{
	return false ;
}

std::string GSmtp::ExecutableFilter::id() const
{
	return m_path.str() ;
}

bool GSmtp::ExecutableFilter::specialCancelled() const
{
	return m_special_cancelled ;
}

bool GSmtp::ExecutableFilter::specialOther() const
{
	return m_special_other ;
}

std::string GSmtp::ExecutableFilter::text() const
{
	if( m_ok ) return std::string() ;
	const std::string default_ = "pre-processing failed" ;
	return m_text.empty() ? default_ : m_text ;
}

void GSmtp::ExecutableFilter::start( const std::string & message_file )
{
	// run the program
	G::Executable commandline( m_path.str() ) ;
	commandline.add( G::Path(message_file).str() ) ;
	G_LOG( "GSmtp::ExecutableFilter::start: running " << commandline.displayString() ) ;
	m_task.start( commandline ) ;
}

void GSmtp::ExecutableFilter::onTaskDone( int exit_code , const std::string & output )
{
	// search the output for diagnostics
	m_text = parseOutput( output ) ;
	G_LOG( "GSmtp::ExecutableFilter::preprocess: exit status " << exit_code << " (\"" << m_text << "\")" ) ;

	// interpret the exit code
	Exit exit( exit_code , m_server_side ) ;
	m_ok = exit.ok ;
	m_special_cancelled = exit.cancelled ;
	m_special_other = exit.other ;
	if( !m_ok )
	{
		G_WARNING( "GSmtp::ExecutableFilter::preprocess: pre-processing failed: exit code " << exit_code ) ;
	}

	// callback
	m_done_signal.emit( m_ok ) ;
}

std::string GSmtp::ExecutableFilter::parseOutput( std::string s ) const
{
	G_DEBUG( "GSmtp::ExecutableFilter::parseOutput: in: \"" << G::Str::printable(s) << "\"" ) ;

	const std::string start_1("<<") ;
	const std::string end_1(">>") ;
	const std::string start_2("[[") ;
	const std::string end_2("]]") ;

	std::string result ;
	while( G::Str::replaceAll( s , "\r\n" , "\n" ) ) ;
	G::Str::replaceAll( s , "\r" , "\n" ) ;
	G::StringArray lines ;
	G::Str::splitIntoFields( s , lines , "\n" ) ;
	for( G::StringArray::iterator p = lines.begin() ; p != lines.end() ; ++p )
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
	G_DEBUG( "GSmtp::ExecutableFilter::parseOutput: out: \"" << G::Str::printable(result) << "\"" ) ;
	return result ;
}

G::Slot::Signal1<bool> & GSmtp::ExecutableFilter::doneSignal()
{
	return m_done_signal ;
}

void GSmtp::ExecutableFilter::cancel()
{
	m_task.stop() ;
}

/// \file gexecutablefilter.cpp
