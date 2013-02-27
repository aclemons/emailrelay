//
// Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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
	return *this ;
}

void GSmtp::MessageStore::Iterator::last()
{
	IteratorImp * imp = m_imp ;
	m_imp = NULL ;
	delete imp ;
}

/// \file gmessagestore.cpp
