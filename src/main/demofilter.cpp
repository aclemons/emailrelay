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
#include "genvelope.h"
#include "groot.h"
#include "gfile.h"
#include "gscope.h"
#include "gstringarray.h"
#include "gassert.h"
#include "glog.h"
#include <stdexcept>

Main::DemoFilter::DemoFilter( GNet::ExceptionSink es , Main::Run & run , unsigned int unit_id ,
	GStore::FileStore & store , GSmtp::Filter::Type filter_type ,
	const GSmtp::Filter::Config & filter_config , const std::string & spec ) :
		m_run(run) ,
		m_unit_id(unit_id) ,
		m_store(store) ,
		m_filter_type(filter_type) ,
		m_filter_config(filter_config) ,
		m_spec(spec) ,
		m_timer(*this,&DemoFilter::onTimeout,es) ,
		m_result(Result::fail)
{
	const Unit & unit = m_run.unit( unit_id ) ;
	G_LOG( "Main::DemoFilter::ctor: demo filter: [" << unit.name() << "] [" << spec << "]" ) ;
}

Main::DemoFilter::~DemoFilter()
= default ;

std::string Main::DemoFilter::id() const
{
	return "demo" ; // ie. "--filter=demo:..."
}

bool Main::DemoFilter::simple() const
{
	return false ; // not trivial
}

void Main::DemoFilter::start( const GStore::MessageId & message_id )
{
	// get file-system paths
	auto envelope_state = m_filter_type == GSmtp::Filter::Type::server ?
		GStore::FileStore::State::New : GStore::FileStore::State::Locked ;
	G::Path envelope_path = m_store.envelopePath( message_id , envelope_state ) ;
	G::Path content_path = m_store.contentPath( message_id ) ;
	G::Path content_path_tmp = content_path.str() + ".tmp" ;
	G_LOG( "Main::DemoFilter::start: demo filter: [" << m_spec << "]: content file: [" << content_path << "]" ) ;

	// read the envelope
	std::ifstream envelope_stream_in ;
	{
		G::Root claim_root ; // (effective userid switch)
		G::File::open( envelope_stream_in , envelope_path ) ;
	}
	GStore::Envelope envelope ;
	GStore::Envelope::read( envelope_stream_in , envelope ) ;
	envelope_stream_in.close() ;

	// open the read and write content streams
	std::ifstream content_stream_in ;
	std::ofstream content_stream_out ;
	{
		G::Root claim_root ;
		G::File::open( content_stream_in , content_path ) ;
		G::File::open( content_stream_out , content_path_tmp ) ;
	}
	G::ScopeExit clean_up( [content_path_tmp](){G::File::remove(content_path_tmp,std::nothrow);} ) ;

	// prepare a new header line
	std::string new_header = std::string("X-MailRelay-Demo: ").append(m_filter_config.domain).append("\r\n",2U) ;

	// copy the content
	bool in_headers = true ;
	std::vector<std::string> headers ;
	std::string line ;
	while( G::Str::readLine( content_stream_in , line ) )
	{
		G::Str::trimRight( line , "\r"_sv ) ;
		if( in_headers )
		{
			if( line.empty() || line.find_first_not_of(" \t") == std::string::npos )
			{
				// blank line -- emit our new header line
				in_headers = false ;
				content_stream_out.write( new_header.data() , new_header.size() ) ;
				line.clear() ;
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

		// optionally edit the body text
		if( !in_headers && m_spec == "shout" )
			G::Str::toUpper( line ) ;

		content_stream_out.write( line.data() , line.size() ) ;
	}

	// close the new content
	content_stream_out.close() ;
	if( !content_stream_out )
		throw G::Exception( "cannot edit content file" , content_path.str() ) ;

	// commit the content
	{
		G::Root claim_root ;
		G::File::rename( content_path_tmp , content_path ) ;
		clean_up.release() ;
	}

	// pick a random recipient
	std::string envelope_to = envelope.to_remote.empty() ? std::string() : envelope.to_remote[0] ;

	// re-write the envelope with a forward-to value
	std::ofstream envelope_stream_out ;
	{
		G::Root claim_root ;
		G::File::open( envelope_stream_out , envelope_path ) ;
	}
	envelope.forward_to = G::Str::tail( envelope_to , "@" ) ;
	GStore::Envelope::write( envelope_stream_out , envelope ) ;
	envelope_stream_out.close() ;
	if( envelope_stream_out.fail() )
		throw std::runtime_error( "cannot re-write the envelope file" ) ;

	// use the timer for the asynchronous completion
	m_result = Result::ok ;
	m_timer.startTimer( 1U ) ;
}

void Main::DemoFilter::onTimeout()
{
	m_done_signal.emit( static_cast<int>(m_result) ) ;
}

G::Slot::Signal<int> & Main::DemoFilter::doneSignal() noexcept
{
	return m_done_signal ;
}

void Main::DemoFilter::cancel()
{
	m_timer.cancelTimer() ;
}

GSmtp::Filter::Result Main::DemoFilter::result() const
{
	return m_result ;
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

