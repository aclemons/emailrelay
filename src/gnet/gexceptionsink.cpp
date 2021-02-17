//
// Copyright (C) 2001-2021 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gexceptionsink.cpp
///

#include "gdef.h"
#include "gnetdone.h"
#include "gexceptionsink.h"
#include "geventloop.h"
#include "gassert.h"

namespace GNet
{
	namespace ExceptionSinkImp /// An implementation namespace for GNet::ExceptionSink.
	{
		struct RethrowExceptionHandler : public GNet::ExceptionHandler /// An GNet::ExceptionHandler that rethrows.
		{
			void onException( GNet::ExceptionSource * , std::exception & , bool ) override
			{
				throw ;
			}
		} rethrow_exception_handler ;
	}
}

GNet::ExceptionSink::ExceptionSink( Type type , ExceptionSource * ) noexcept :
	m_eh(type==Type::Null?nullptr:&ExceptionSinkImp::rethrow_exception_handler)
{
}

GNet::ExceptionSink::ExceptionSink( ExceptionHandler & eh , ExceptionSource * esrc ) noexcept :
	m_eh(&eh) ,
	m_esrc(esrc)
{
}

GNet::ExceptionSink::ExceptionSink( ExceptionHandler * eh , ExceptionSource * esrc ) noexcept :
	m_eh(eh) ,
	m_esrc(esrc)
{
}

GNet::ExceptionHandler * GNet::ExceptionSink::eh() const noexcept
{
	return m_eh ;
}

GNet::ExceptionSource * GNet::ExceptionSink::esrc() const noexcept
{
	return m_esrc ;
}

void GNet::ExceptionSink::call( std::exception & e , bool done )
{
	if( m_eh != nullptr )
	{
		m_eh->onException( m_esrc , e , done ) ;
	}
}

void GNet::ExceptionSink::reset() noexcept
{
	m_eh = nullptr ;
	m_esrc = nullptr ;
}

bool GNet::ExceptionSink::set() const noexcept
{
	return m_eh != nullptr ;
}

// ==

GNet::ExceptionSinkUnbound::ExceptionSinkUnbound( ExceptionHandler & eh ) :
	m_eh(&eh)
{
}

GNet::ExceptionSinkUnbound::ExceptionSinkUnbound( ExceptionHandler * eh ) :
	m_eh(eh)
{
	G_ASSERT( eh != nullptr ) ;
}

GNet::ExceptionSink GNet::ExceptionSinkUnbound::bind( ExceptionSource * source ) const
{
	return ExceptionSink{ m_eh , source } ;
}

