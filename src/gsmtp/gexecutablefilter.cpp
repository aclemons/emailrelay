//
// Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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
///
/// \file gexecutablefilter.cpp
///

#include "gdef.h"
#include "gprocess.h"
#include "gnewprocess.h"
#include "gexecutablecommand.h"
#include "gexecutablefilter.h"
#include "gstr.h"
#include "groot.h"
#include "glog.h"
#include "gassert.h"
#include <tuple>

GSmtp::ExecutableFilter::ExecutableFilter( GNet::ExceptionSink es ,
	FileStore & file_store , bool server_side , const std::string & path ,
	unsigned int timeout ) :
		m_file_store(file_store) ,
		m_server_side(server_side) ,
		m_prefix(server_side?"filter":"client filter") ,
		m_exit(0,server_side) ,
		m_path(path) ,
		m_timeout(timeout) ,
		m_timer(*this,&ExecutableFilter::onTimeout,es) ,
		m_task(*this,es,"<<filter exec error: __strerror__>>",G::Root::nobody())
{
}

GSmtp::ExecutableFilter::~ExecutableFilter()
= default;

bool GSmtp::ExecutableFilter::simple() const
{
	return false ;
}

std::string GSmtp::ExecutableFilter::id() const
{
	return m_path.basename() ; // was .str()
}

bool GSmtp::ExecutableFilter::special() const
{
	return m_exit.special ;
}

bool GSmtp::ExecutableFilter::abandoned() const
{
	return m_exit.abandon() ;
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

void GSmtp::ExecutableFilter::start( const MessageId & message_id )
{
	FileStore::State state = m_server_side ? FileStore::State::New : FileStore::State::Locked ;
	G::Path cpath = m_file_store.contentPath( message_id ) ;
	G::Path epath = m_file_store.envelopePath( message_id , state ) ;

	G::StringArray args ;
	args.push_back( cpath.str() ) ;
	args.push_back( epath.str() ) ;
	G::ExecutableCommand commandline( m_path.str() , args ) ;
	G_LOG( "GSmtp::ExecutableFilter::start: " << m_prefix << ": running " << commandline.displayString() ) ;
	m_task.start( commandline ) ;

	if( m_timeout )
		m_timer.startTimer( m_timeout ) ;
}

void GSmtp::ExecutableFilter::onTimeout()
{
	G_WARNING( "GSmtp::ExecutableFilter::onTimeout: " << m_prefix << " timed out after " << m_timeout << "s" ) ;
	m_task.stop() ;
	m_exit = Exit( 1 , m_server_side ) ;
	G_ASSERT( m_exit.fail() && !special() && !abandoned() ) ;
	m_response = "error" ;
	m_reason = "timeout" ;
	m_done_signal.emit( static_cast<int>(m_exit.result) ) ;
}

void GSmtp::ExecutableFilter::onTaskDone( int exit_code , const std::string & output )
{
	m_timer.cancelTimer() ;

	// search the output for diagnostics
	std::tie(m_response,m_reason) = parseOutput( output , "rejected" ) ;
	if( m_response.find("filter exec error") == 0U ) // see ctor
	{
		m_reason = m_response ;
		m_response = "rejected" ;
	}

	m_exit = Exit( exit_code , m_server_side ) ;
	if( !m_exit.ok() )
	{
		G_WARNING( "GSmtp::ExecutableFilter::onTaskDone: " << m_prefix << " failed: "
			<< "exit code " << exit_code << ": [" << m_response << "]" ) ;
	}

	// callback
	m_done_signal.emit( static_cast<int>(m_exit.result) ) ;
}

std::pair<std::string,std::string> GSmtp::ExecutableFilter::parseOutput( std::string s ,
	const std::string & default_ ) const
{
	G_DEBUG( "GSmtp::ExecutableFilter::parseOutput: in: \"" << G::Str::printable(s) << "\"" ) ;

	static constexpr auto start_1 = "<<"_sv ;
	static constexpr auto end_1 = ">>"_sv ;
	static constexpr auto start_2 = "[["_sv ;
	static constexpr auto end_2 = "]]"_sv ;

	G::StringArray lines ;
	while( G::Str::replaceAll( s , "\r\n" , "\n" ) ) {;}
	G::Str::replaceAll( s , "\r" , "\n" ) ;
	G::Str::splitIntoFields( s , lines , '\n' ) ;

	for( auto p = lines.begin() ; p != lines.end() ; )
	{
		const std::string & line = *p ;
		std::size_t pos_start = line.find( start_1.data() , 0U , start_1.size() ) ;
		std::size_t pos_end = line.find( end_1.data() , 0U , end_1.size() ) ;
		if( pos_start != 0U )
		{
			pos_start = line.find( start_2.data() , 0U , start_2.size() ) ;
			pos_end = line.find( end_2.data() , 0U , end_2.size() ) ;
		}
		if( pos_start == 0U && pos_end != std::string::npos )
		{
			*p++ = G::Str::printable( line.substr(2U,pos_end-2U) ) ;
		}
		else
		{
			p = lines.erase( p ) ;
		}
	}

	G_DEBUG( "GSmtp::ExecutableFilter::parseOutput: out: [" << G::Str::join("|",lines) << "]" ) ;

	std::string response = ( !lines.empty() && !lines.at(0U).empty() ) ? lines.at(0U) : default_ ;
	std::string reason = ( lines.size() > 1U && !lines.at(1U).empty() ) ? lines.at(1U) : response ;
	return { response , reason } ;
}

G::Slot::Signal<int> & GSmtp::ExecutableFilter::doneSignal()
{
	return m_done_signal ;
}

void GSmtp::ExecutableFilter::cancel()
{
	m_task.stop() ;
	m_timer.cancelTimer() ;
}

