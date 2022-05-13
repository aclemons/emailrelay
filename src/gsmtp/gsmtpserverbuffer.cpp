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
/// \file gsmtpserverbuffer.cpp
///

#include "gdef.h"
#include "gsmtpserverbuffer.h"
#include "gstr.h"
#include "glog.h"
#include "gassert.h"
#include <string>

GSmtp::ServerBuffer::ServerBuffer( GNet::ExceptionSink es , ServerProtocol & protocol ,
	ServerSender & sender , std::size_t line_buffer_limit , size_t pipelining_buffer_limit ,
	bool enable_batching ) :
		m_timer(*this,&ServerBuffer::onTimeout,es) ,
		m_protocol(protocol) ,
		m_sender(sender) ,
		m_line_buffer(GNet::LineBufferConfig::smtp()) ,
		m_line_buffer_limit(line_buffer_limit) ,
		m_pipelining_buffer_limit(pipelining_buffer_limit) ,
		m_enable_batching(enable_batching) ,
		m_blocked(false)
{
	m_protocol.setSender( *this ) ;
}

void GSmtp::ServerBuffer::apply( const char * data , std::size_t size )
{
	if( data == nullptr || size == 0U )
		return ;

	// limit on long lines with no CRLF
	if( m_line_buffer_limit && !m_protocol.inDataState() &&
		(m_line_buffer.buffersize()+size) > m_line_buffer_limit )
	{
		doOverflow( "input" ) ;
	}

	if( !m_line_buffer.apply( std::bind(&ServerProtocol::apply,&m_protocol,std::placeholders::_1) ,
		{data,size} ,
		std::bind(&ServerProtocol::inDataState,&m_protocol) ) )
	{
		m_timer.startTimer( G::TimeInterval::limit() ) ;
	}
}

bool GSmtp::ServerBuffer::protocolSend( const std::string & line , bool flush )
{
	if( m_blocked || ( !flush && m_enable_batching ) )
	{
		G_DEBUG( "GSmtp::ServerBuffer::protocolSend: queue line=[" << G::Str::printable(line) << "]: b=" << m_blocked << " f=" << flush ) ;
		check( line.size() ) ;
		m_batch.append( line ) ;
	}
	else if( !m_batch.empty() )
	{
		check( line.size() ) ;
		m_batch.append( line ) ;
		std::string batch ;
		std::swap( batch , m_batch ) ;
		G_DEBUG( "GSmtp::ServerBuffer::protocolSend: flush batch=[" << G::Str::printable(batch) << "]" ) ;
		if( !m_sender.protocolSend( batch , true ) )
			m_blocked = true ;
	}
	else if( !line.empty() )
	{
		G_DEBUG( "GSmtp::ServerBuffer::protocolSend: send line=[" << G::Str::printable(line) << "]" ) ;
		if( !m_sender.protocolSend( line , true ) )
			m_blocked = true ;
	}

	// apply any residue left in the input line buffer
	if( m_timer.active() )
		m_timer.startTimer( 0U ) ;

	G_ASSERT( !flush || ( m_blocked || m_batch.empty() ) ) ;
	return true ;
}

void GSmtp::ServerBuffer::sendComplete()
{
	G_DEBUG( "GSmtp::ServerBuffer::protocolSend: unblocked batch=[" << G::Str::printable(m_batch) << "]" ) ;
	if( !m_blocked )
		return ; // see protocolSecure()

	m_blocked = false ;
	std::string batch ;
	std::swap( batch , m_batch ) ;

	if( !batch.empty() && !m_sender.protocolSend( batch , true ) )
		m_blocked = true ;
}

void GSmtp::ServerBuffer::onTimeout()
{
	if( !m_line_buffer.apply( std::bind(&ServerProtocol::apply,&m_protocol,std::placeholders::_1) ,
		{nullptr,std::size_t(0U)} ,
		std::bind(&ServerProtocol::inDataState,&m_protocol) ) )
	{
		m_timer.startTimer( G::TimeInterval::limit() ) ;
	}
}

void GSmtp::ServerBuffer::protocolSecure()
{
	m_blocked = false ;
	m_batch.clear() ;
	m_sender.protocolSecure() ;
}

void GSmtp::ServerBuffer::check( std::size_t n )
{
	if( m_pipelining_buffer_limit && (m_batch.size()+n) > m_pipelining_buffer_limit )
		doOverflow( "output" ) ;
}

void GSmtp::ServerBuffer::doOverflow( const std::string & direction )
{
	G_WARNING( "GSmtp::ServerBuffer: buffer overflow on " << direction ) ;
	m_blocked = false ;
	m_batch.clear() ;
	m_timer.cancelTimer() ;
	m_sender.protocolSend( "500 buffer overflow on " + direction + "\r\n" , true ) ;
	throw ServerProtocol::Done( "buffer overflow" ) ;
}

void GSmtp::ServerBuffer::protocolShutdown( int how )
{
	m_timer.cancelTimer() ;
	m_sender.protocolShutdown( how ) ;
}

void GSmtp::ServerBuffer::protocolExpect( std::size_t n )
{
	m_line_buffer.expect( n ) ;
}

std::string GSmtp::ServerBuffer::head() const
{
	return m_line_buffer.state().head() ;
}

