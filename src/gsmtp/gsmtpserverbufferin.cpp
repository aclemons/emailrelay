//
// Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gsmtpserverbufferin.cpp
///

#include "gdef.h"
#include "gsmtpserverbufferin.h"
#include "glog.h"
#include "gassert.h"

GSmtp::ServerBufferIn::ServerBufferIn( GNet::ExceptionSink es , ServerProtocol & protocol ,
	const Config & config ) :
		m_protocol(protocol) ,
		m_config(config) ,
		m_line_buffer(GNet::LineBuffer::Config::smtp()) ,
		m_timer(*this,&ServerBufferIn::onTimeout,es)
{
	m_protocol.changeSignal().connect( G::Slot::slot(*this,&ServerBufferIn::onProtocolChange) ) ;
}

GSmtp::ServerBufferIn::~ServerBufferIn()
{
	m_protocol.changeSignal().disconnect() ;
}

void GSmtp::ServerBufferIn::apply( const char * data , std::size_t size )
{
	applySome( data , size ) ;
	if( m_timer.active() && overLimit() )
		flowOff() ;
}

void GSmtp::ServerBufferIn::onTimeout()
{
	applySome( nullptr , 0U ) ;
	if( !m_timer.active() )
		flowOn() ;
}

void GSmtp::ServerBufferIn::applySome( const char * data , std::size_t size )
{
	if( m_protocol.inBusyState() )
	{
		G_ASSERT( m_timer.active() ) ;
		m_line_buffer.add( data , size ) ;
	}
	else if( !m_line_buffer.apply( std::bind(&ServerProtocol::apply,&m_protocol,std::placeholders::_1) ,
		{data,size} , std::bind(&ServerProtocol::inDataState,&m_protocol) ) )
	{
		// ServerProtocol::apply() returned false
		G_ASSERT( m_protocol.inBusyState() ) ;
		m_timer.startTimer( G::TimeInterval::limit() ) ;
	}
	else
	{
		m_timer.cancelTimer() ;
	}
	G_ASSERT( m_timer.active() == m_protocol.inBusyState() ) ;
	if( overHardLimit() )
		throw Overflow() ; // (if flow-control is not working)
}

bool GSmtp::ServerBufferIn::overLimit() const
{
	return m_line_buffer.buffersize() >= std::max(std::size_t(1U),m_config.input_buffer_soft_limit) ;
}

bool GSmtp::ServerBufferIn::overHardLimit() const
{
	return m_config.input_buffer_hard_limit && m_line_buffer.buffersize() >= m_config.input_buffer_hard_limit ;
}

void GSmtp::ServerBufferIn::flowOn()
{
	if( !m_flow_on )
	{
		m_flow_on = true ;
		m_flow_signal.emit( true ) ;
	}
}

void GSmtp::ServerBufferIn::flowOff()
{
	if( m_flow_on )
	{
		m_flow_on = false ;
		m_flow_signal.emit( false ) ;
	}
}

void GSmtp::ServerBufferIn::onProtocolChange()
{
	if( m_timer.active() )
		m_timer.startTimer( 0U ) ;
}

void GSmtp::ServerBufferIn::expect( std::size_t n )
{
	return m_line_buffer.expect( n ) ;
}

std::string GSmtp::ServerBufferIn::head() const
{
	return m_line_buffer.state().head() ;
}

G::Slot::Signal<bool> & GSmtp::ServerBufferIn::flowSignal() noexcept
{
	return m_flow_signal ;
}

