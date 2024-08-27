//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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

// use a static string here for run-time efficiency -- however, it does
// make the effects of an inner nested object persist beyond its scope --
// in practice that is not a problem because the event-loop's outer object
// and any inner object are both destroyed in quick succession
std::string GNet::EventLoggingContext::m_s ;

#ifndef G_LIB_SMALL
GNet::EventLoggingContext::EventLoggingContext( std::string_view s ) :
	m_outer(m_inner)
{
	m_s.assign( s.data() , s.size() ) ;

	G::LogOutput::Instance::context( EventLoggingContext::fn , this ) ;
	m_inner = this ;
}
#endif

GNet::EventLoggingContext::EventLoggingContext( EventState es , const std::string & s ) :
	m_outer(m_inner)
{
	set( m_s , es ) ;
	m_s.append( s ) ;

	G::LogOutput::Instance::context( EventLoggingContext::fn , this ) ;
	m_inner = this ;
}

GNet::EventLoggingContext::EventLoggingContext( EventState es ) :
	m_outer(m_inner)
{
	set( m_s , es ) ;

	G::LogOutput::Instance::context( EventLoggingContext::fn , this ) ;
	m_inner = this ;
}

void GNet::EventLoggingContext::set( std::string & s , EventState es )
{
	s.clear() ; // (static instance for run-time efficiency)
	for( const EventLogging * p = es.logging() ; p ; p = p->next() )
	{
		if( !p->eventLoggingString().empty() )
		{
			std::string_view sv = p->eventLoggingString() ;
			s.insert( 0U , sv.data() , sv.size() ) ;
		}
	}
}

GNet::EventLoggingContext::~EventLoggingContext()
{
	if( m_outer )
		G::LogOutput::Instance::context( EventLoggingContext::fn , m_outer ) ;
	else
		G::LogOutput::Instance::context() ;
	m_inner = m_outer ;
}

std::string_view GNet::EventLoggingContext::fn( void * )
{
	if( m_inner == nullptr ) return {} ;
	return m_s ; // m_inner->m_s if non-static
}

