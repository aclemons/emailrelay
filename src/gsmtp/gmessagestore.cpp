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
// gmessagestore.cpp
//

#include "gdef.h"
#include "gsmtp.h"
#include "gmessagestore.h"
#include "glog.h"
#include "gassert.h"

GSmtp::MessageStore::~MessageStore()
{
}

// ===

GSmtp::MessageStore::IteratorImp::IteratorImp() :
	m_ref_count(1UL)
{
}

GSmtp::MessageStore::IteratorImp::~IteratorImp()
{
}

// ===

GSmtp::MessageStore::Iterator::Iterator() : 
	m_imp(NULL)
{
}

GSmtp::MessageStore::Iterator::Iterator( IteratorImp * imp ) :
	m_imp(imp)
{
	G_ASSERT( m_imp->m_ref_count == 1UL ) ;
}

std::auto_ptr<GSmtp::StoredMessage> GSmtp::MessageStore::Iterator::next()
{
	return m_imp ? m_imp->next() : std::auto_ptr<StoredMessage>(NULL) ;
}

GSmtp::MessageStore::Iterator::~Iterator()
{
	if( m_imp )
	{
		m_imp->m_ref_count-- ;
		if( m_imp->m_ref_count == 0UL )
			delete m_imp ;
	}
}

GSmtp::MessageStore::Iterator::Iterator( const Iterator & other ) :
	m_imp(other.m_imp)
{
	if( m_imp )
		m_imp->m_ref_count++ ;
}

GSmtp::MessageStore::Iterator & GSmtp::MessageStore::Iterator::operator=( const Iterator & rhs )
{
	if( this != &rhs )
	{
		if( m_imp )
		{
			m_imp->m_ref_count-- ;
			if( m_imp->m_ref_count == 0UL )
				delete m_imp ;
		}
		m_imp = rhs.m_imp ;
		if( m_imp )
		{
			m_imp->m_ref_count++ ;
		}
	}
	return * this ;
}

