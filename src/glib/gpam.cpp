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
// gpam.cpp
//

#include "gdef.h"
#include "gpam.h"
#include "gexception.h"
#include "gstr.h"
#include "gdebug.h"

G::Pam::Error::Error( const std::string & op , int rc ) :
	G::Exception("pam error") ,
	m_pam_error(rc)
{
	append( op ) ;
	append( Str::fromInt(rc) ) ;
	G_DEBUG( "G::Pam::Error::ctor: " << m_what ) ;
}

G::Pam::Error::Error( const std::string & op , int rc , const char * more ) :
	G::Exception("pam error") ,
	m_pam_error(rc)
{
	append( op ) ;
	append( Str::fromInt(rc) ) ;
	append( more ) ;
	G_DEBUG( "G::Pam::Error::ctor: " << m_what ) ;
}

/// \file gpam.cpp
