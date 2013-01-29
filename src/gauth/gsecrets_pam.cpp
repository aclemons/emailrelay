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
// gsecrets_pam.cpp
//

#include "gdef.h"
#include "gauth.h"
#include "gsecrets.h"
#include "gsecretsfile.h"
#include "gdebug.h"

GAuth::Secrets::Secrets( const std::string & path , const std::string & name , const std::string & type ) :
	m_source(path) ,
	m_imp(NULL)
{
	G_DEBUG( "GAuth::Secrets:ctor: [" << path << "]" ) ;
	if( m_source != "/pam" )
		m_imp = new SecretsFile(path,true,name,type) ;
}

GAuth::Secrets::Secrets() :
	m_imp(NULL)
{
	if( m_source != "/pam" )
		m_imp = new SecretsFile(std::string(),true,std::string(),std::string()) ;
}

GAuth::Secrets::~Secrets()
{
	delete m_imp ;
}

std::string GAuth::Secrets::source() const
{
	return m_source ;
}

bool GAuth::Secrets::valid() const
{
	return m_source == "/pam" || m_imp->valid() ;
}

std::string GAuth::Secrets::id( const std::string & mechanism ) const
{
	return valid() ? m_imp->id( mechanism ) : std::string() ;
}

std::string GAuth::Secrets::secret( const std::string & mechanism ) const
{
	return valid() ? m_imp->secret( mechanism ) : std::string() ;
}

std::string GAuth::Secrets::secret( const std::string & mechanism , const std::string & id ) const
{
	return valid() ? m_imp->secret( mechanism , id ) : std::string() ;
}

bool GAuth::Secrets::contains( const std::string & mechanism ) const
{
	return valid() ? m_imp->contains( mechanism ) : false ;
}

/// \file gsecrets_pam.cpp
