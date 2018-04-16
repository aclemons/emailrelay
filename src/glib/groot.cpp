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
// groot.cpp
//

#include "gdef.h"
#include "groot.h"
#include "gtest.h"
#include "gassert.h"
#include "gprocess.h"
#include "gdebug.h"

G::Root * G::Root::m_this = nullptr ;
bool G::Root::m_initialised = false ;
bool G::Root::m_default_change_group = true ;
G::Identity G::Root::m_special( G::Identity::invalid() ) ;
G::Identity G::Root::m_ordinary( G::Identity::invalid() ) ;

G::Root::Root() :
	m_change_group(m_default_change_group)
{
	if( G::Test::enabled("root-scope") && m_this != nullptr )
		G_WARNING( "G::Root::ctor: root control object exists at outer scope" ) ;

	if( m_this == nullptr && m_initialised )
	{
		Process::beSpecial( m_special , m_change_group ) ;
		m_this = this ;
	}
}

G::Root::Root( bool change_group ) :
	m_change_group(change_group)
{
	if( G::Test::enabled("root-scope") && m_this != nullptr )
		G_WARNING( "G::Root::ctor: root control object exists at outer scope" ) ;

	if( m_this == nullptr && m_initialised )
	{
		Process::beSpecial( m_special , m_change_group ) ;
		m_this = this ;
	}
}

G::Root::~Root()
{
	try
	{
		if( m_this == this && m_initialised )
		{
			m_this = nullptr ;
			Process::beOrdinary( m_ordinary , m_change_group ) ;
		}
	}
	catch( std::exception & e )
	{
		G_ERROR( "G::Root: cannot release root privileges: " << e.what() ) ;
	}
	catch(...)
	{
		G_ERROR( "G::Root: cannot release root privileges" ) ;
	}
}

G::Identity G::Root::start( SignalSafe safe )
{
	if( !m_initialised ) return Identity::invalid( safe ) ;
	return Process::beSpecial( safe , m_special , m_default_change_group ) ;
}

void G::Root::stop( SignalSafe safe , Identity identity )
{
	if( identity == Identity::invalid(safe) ) return ;
	Process::beOrdinary( safe , identity , m_default_change_group ) ;
}

void G::Root::init( const std::string & non_root , bool default_change_group )
{
	G_ASSERT( !non_root.empty() ) ;
	Process::revokeExtraGroups() ;
	m_ordinary = Identity( non_root ) ;
	m_special = Process::beOrdinary( SignalSafe() , m_ordinary , default_change_group ) ;
	m_initialised = true ;
	m_default_change_group = default_change_group ;
}

G::Identity G::Root::nobody()
{
	return m_ordinary ;
}

/// \file groot.cpp
