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
/// \file demofilter.cpp
///

#include "gdef.h"
#include "demofilter.h"
#include "groot.h"
#include "gfile.h"
#include "gscope.h"
#include "gstringarray.h"

Main::DemoFilter::DemoFilter( GNet::ExceptionSink es , Main::Run & , unsigned int /*unit_id*/ ,
	GStore::FileStore & store , const std::string & spec ) :
		m_store(store) ,
		m_spec(spec) ,
		m_timer(*this,&DemoFilter::onTimeout,es)
{
}

Main::DemoFilter::~DemoFilter()
= default ;

std::string Main::DemoFilter::id() const
{
	return "demo" ;
}

bool Main::DemoFilter::simple() const
{
	return false ;
}

void Main::DemoFilter::start( const GStore::MessageId & message_id )
{
	G::Path content_path = m_store.contentPath( message_id ) ;
	G::Path content_path_tmp = content_path.str() + ".tmp" ;

	G_LOG( "Main::DemoFilter::start: demo filter: [" << m_spec << "]: content file: [" << content_path << "]" ) ;

	std::ifstream content_in ;
	std::ofstream content_out ;
	{
		G::Root claim_root ;
		G::File::open( content_in , content_path ) ;
		G::File::open( content_out , content_path_tmp ) ;
	}
	G::ScopeExit clean_up( [content_path_tmp](){G::File::remove(content_path_tmp,std::nothrow);} ) ;

	bool in_headers = true ;
	std::vector<std::string> headers ;

	std::string line ;
	while( G::Str::readLine( content_in , line ) )
	{
		G::Str::trimRight( line , "\r"_sv ) ;
		if( in_headers )
		{
			if( line.empty() || line.find_first_not_of(" \t") == std::string::npos )
			{
				in_headers = false ;
				line.clear() ;
				content_out.write( "X-Demo: 1\r\n" , 11U ) ;
			}
			else if( ( line[0] == ' ' || line[0] == '\t' ) && !headers.empty() )
			{
				headers.back().append( line ) ; // unfold
			}
			else
			{
				headers.push_back( line ) ;
			}
		}
		line.append( "\r\n" , 2U ) ;

		if( !in_headers && m_spec == "shout" )
			G::Str::toUpper( line ) ;

		content_out.write( line.data() , line.size() ) ;
	}

	content_out.close() ;
	if( !content_out )
		throw G::Exception( "cannot edit content file" , content_path.str() ) ;

	{
		G::Root claim_root ;
		G::File::rename( content_path_tmp , content_path ) ;
		clean_up.release() ;
	}

	m_timer.startTimer( 1U ) ;
}

void Main::DemoFilter::onTimeout()
{
	m_done_signal.emit( 0 ) ;
}

G::Slot::Signal<int> & Main::DemoFilter::doneSignal()
{
	return m_done_signal ;
}

void Main::DemoFilter::cancel()
{
	m_timer.cancelTimer() ;
}

bool Main::DemoFilter::abandoned() const
{
	return false ;
}

std::string Main::DemoFilter::response() const
{
	return {} ;
}

std::string Main::DemoFilter::reason() const
{
	return {} ;
}

bool Main::DemoFilter::special() const
{
	return false ;
}

