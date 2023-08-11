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
/// \file geventloggingcontext.cpp
///

#include "gdef.h"
#include "geventloggingcontext.h"
#include "gexceptionsource.h"
#include "glogoutput.h"

GNet::EventLoggingContext * GNet::EventLoggingContext::m_inner = nullptr ;

GNet::EventLoggingContext::EventLoggingContext( ExceptionSource * esrc ) :
	m_outer(m_inner) ,
	m_esrc(esrc)
{
	m_inner = this ;
	G::LogOutput::context( EventLoggingContext::fn , this ) ;
}

GNet::EventLoggingContext::EventLoggingContext( const std::string & s ) :
	m_outer(m_inner) ,
	m_esrc(nullptr) ,
	m_s(s)
{
	m_inner = this ;
	G::LogOutput::context( EventLoggingContext::fn , this ) ;
}

GNet::EventLoggingContext::~EventLoggingContext() noexcept
{
	if( m_outer )
		G::LogOutput::context( EventLoggingContext::fn , m_outer ) ;
	else
		G::LogOutput::context() ;
	m_inner = m_outer ;
}

std::string GNet::EventLoggingContext::fn( void * vp )
{
	std::string s = static_cast<EventLoggingContext*>(vp)->str() ;
	if( !s.empty() )
		s.append( "; " , 2U ) ; // semi-colon for simpler fail2ban regexpes
	return s ;
}

std::string GNet::EventLoggingContext::str()
{
	return m_esrc ? m_esrc->exceptionSourceId() : m_s ;
}

