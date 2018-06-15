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
#include "gexecutablecommand.h"
#include "gexecutablefilter.h"
#include "gstr.h"
#include "groot.h"
#include "gdebug.h"

GSmtp::ExecutableFilter::ExecutableFilter( GNet::ExceptionHandler & eh ,
	bool server_side , const std::string & path ) :
		m_server_side(server_side) ,
		m_prefix(server_side?"filter":"client filter") ,
		m_exit(0,server_side) ,
		m_path(path) ,
		m_task(*this,eh,"<<filter exec error: __strerror__>>",G::Root::nobody())
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

bool GSmtp::ExecutableFilter::special() const
{
	return m_exit.special ;
}

std::string GSmtp::ExecutableFilter::response() const
{
	G_ASSERT( m_exit.ok() || m_exit.abandon() || !m_response.empty() ) ;
	if( m_exit.ok() || m_exit.abandon() )
		return std::string() ;
	else
		return m_response ;
}

std::string GSmtp::ExecutableFilter::reason() const
{
	G_ASSERT( m_exit.ok() || m_exit.abandon() || !m_reason.empty() ) ;
	if( m_exit.ok() || m_exit.abandon() )
		return std::string() ;
	else
		return m_reason ;
}

void GSmtp::ExecutableFilter::start( const std::string & message_file )
{
	// run the program
	G::StringArray args ;
	args.push_back( G::Path(message_file).str() ) ;
	G::ExecutableCommand commandline( m_path.str() , args ) ;
	G_LOG( "GSmtp::ExecutableFilter::start: " << m_prefix << ": running " << commandline.displayString() ) ;
	m_task.start( commandline ) ;
}

void GSmtp::ExecutableFilter::onTaskDone( int exit_code , const std::string & output )
{
	// search the output for diagnostics
	std::pair<std::string,std::string> pair = parseOutput( output , "rejected" ) ;
	m_response = pair.first ;
	m_reason = pair.second ;
	if( m_response.find("filter exec error") == 0U ) // see ctor
	{
		m_reason = m_response ;
		m_response = "rejected" ;
	}

	m_exit = Exit( exit_code , m_server_side ) ;
	if( !m_exit.ok() )
	{
		G_WARNING( "GSmtp::ExecutableFilter::onTaskDone: " << m_prefix << " failed: "
			<< "exit code " << exit_code << " [" << m_response << "]" ) ;
	}

	// callback
	m_done_signal.emit( m_exit.result ) ;
}

std::pair<std::string,std::string> GSmtp::ExecutableFilter::parseOutput( std::string s , const std::string & default_ ) const
{
	G_DEBUG( "GSmtp::ExecutableFilter::parseOutput: in: \"" << G::Str::printable(s) << "\"" ) ;

	const std::string start_1("<<") ;
	const std::string end_1(">>") ;
	const std::string start_2("[[") ;
	const std::string end_2("]]") ;

	G::StringArray lines ;
	while( G::Str::replaceAll( s , "\r\n" , "\n" ) ) ;
	G::Str::replaceAll( s , "\r" , "\n" ) ;
	G::Str::splitIntoFields( s , lines , "\n" ) ;

	for( G::StringArray::iterator p = lines.begin() ; p != lines.end() ; )
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
			*p++ = G::Str::printable(line.substr(2U,pos_end-2U)) ;
		}
		else
		{
			p = lines.erase( p ) ;
		}
	}

	G_DEBUG( "GSmtp::ExecutableFilter::parseOutput: out: [" << G::Str::join("|",lines) << "]" ) ;

	std::string response = ( !lines.empty() && !lines.at(0U).empty() ) ? lines.at(0U) : default_ ;
	std::string reason = ( lines.size() > 1U && !lines.at(1U).empty() ) ? lines.at(1U) : response ;
	return std::make_pair( response , reason ) ;
}

G::Slot::Signal1<int> & GSmtp::ExecutableFilter::doneSignal()
{
	return m_done_signal ;
}

void GSmtp::ExecutableFilter::cancel()
{
	m_task.stop() ;
}

bool GSmtp::ExecutableFilter::abandoned() const
{
	return m_exit.abandon() ;
}

/// \file gexecutablefilter.cpp
