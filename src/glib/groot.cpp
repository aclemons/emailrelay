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
/// \file groot.cpp
///

#include "gdef.h"
#include "groot.h"
#include "gcleanup.h"
#include "gtest.h"
#include "gassert.h"
#include "gprocess.h"
#include "glog.h"

G::Root * G::Root::m_this = nullptr ;
bool G::Root::m_initialised = false ;
bool G::Root::m_fixed_group = false ;
G::Identity G::Root::m_nobody( G::Identity::invalid() ) ;
G::Identity G::Root::m_startup( G::Identity::invalid() ) ;

G::Root::Root() :
	m_change_group(!m_fixed_group)
{
	check() ;
	if( m_this == nullptr && m_initialised )
	{
		Process::beSpecial( m_startup , m_change_group ) ;
		m_this = this ;
	}
}

G::Root::Root( bool change_group ) :
	m_change_group(m_fixed_group?false:change_group)
{
	check() ;
	if( m_this == nullptr && m_initialised )
	{
		Process::beSpecial( m_startup , m_change_group ) ;
		m_this = this ;
	}
}

G::Root::~Root() // NOLINT bugprone-exception-escape
{
	if( m_this == this && m_initialised )
	{
		m_this = nullptr ;
		int e_saved = Process::errno_() ;
		Process::beOrdinary( m_nobody , m_change_group ) ; // can throw - std::terminate is correct
		Process::errno_( e_saved ) ;
	}
}

void G::Root::atExit() noexcept
{
	if( m_initialised )
		Process::beSpecialForExit( SignalSafe() , m_startup ) ;
}

void G::Root::atExit( SignalSafe safe ) noexcept
{
	if( m_initialised )
		Process::beSpecialForExit( safe , m_startup ) ;
}

void G::Root::init( const std::string & nobody , bool fixed_group )
{
	auto pair = Process::beOrdinaryAtStartup( nobody , !fixed_group ) ;
	m_nobody = pair.first ;
	m_startup = pair.second ;
	m_fixed_group = fixed_group ;
	m_initialised = true ;
}

G::Identity G::Root::nobody()
{
	return m_nobody ;
}

void G::Root::check()
{
	if( G::Test::enabled("root-scope") && m_this != nullptr )
		G_WARNING( "G::Root::check: root control object exists at outer scope" ) ;
}

