//
// Copyright (C) 2001-2004 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later
// version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
// ===
//
// groot.cpp
//

#include "gdef.h"
#include "groot.h"
#include "gprocess.h"
#include "gdebug.h"

G::Root * G::Root::m_this = NULL ;
G::Identity G::Root::m_special( G::Identity::invalid() ) ;
G::Identity G::Root::m_nobody( G::Identity::invalid() ) ;

G::Root::Root( bool change_group ) :
	m_change_group(change_group)
{
	if( m_this == NULL )
	{
		Process::beSpecial( m_special , m_change_group ) ;
		m_this = this ;
	}
}

G::Root::~Root()
{
	try
	{
		if( m_this == this )
		{
			m_this = NULL ;
			Process::beOrdinary( m_nobody , m_change_group ) ;
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

//static
void G::Root::init( const std::string & nobody )
{
	Process::revokeExtraGroups() ;
	m_nobody = nobody.empty() ? Identity::invalid() : Identity(nobody) ;
	m_special = Process::beOrdinary( m_nobody ) ;
}

//static
G::Identity G::Root::nobody()
{
	return m_nobody ;
}

