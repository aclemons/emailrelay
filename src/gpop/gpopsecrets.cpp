//
// Copyright (C) 2001-2005 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gpopsecrets.cpp
//

#include "gdef.h"
#include "gpop.h"
#include "gpopsecrets.h"
#include "gsecrets.h" // gsmtp

// Class: GPop::SecretsImp
// Description: A private pimple-pattern implementation class used by GPop::Secrets.
// The implementation delegates to GSmtp::Secrets.
//
class GPop::SecretsImp 
{
public:
	explicit SecretsImp( const std::string & path ) ;
	bool valid() const ;
	std::string path() const ;
	std::string secret(  const std::string & mechanism , const std::string & id ) const ;
	std::string m_path ;
	GSmtp::Secrets m_secrets ;
} ;

// ===

std::string GPop::Secrets::defaultPath()
{
	return "/etc/emailrelay.auth" ; // TODO: win32
}

GPop::Secrets::Secrets( const std::string & path ) :
	m_imp( new SecretsImp(path) )
{
}

GPop::Secrets::~Secrets()
{
	delete m_imp ;
}

std::string GPop::Secrets::path() const
{
	return m_imp->path() ;
}

bool GPop::Secrets::valid() const
{
	return m_imp->valid() ;
}

std::string GPop::Secrets::secret(  const std::string & mechanism , const std::string & id ) const
{
	return m_imp->secret( mechanism , id ) ;
}

const GSmtp::Secrets & GPop::Secrets::smtp() const
{
	return m_imp->m_secrets ;
}

// ===

GPop::SecretsImp::SecretsImp( const std::string & path ) :
	m_path(path) ,
	m_secrets( path , "pop-server" )
{
}

std::string GPop::SecretsImp::path() const
{
	return m_path ;
}

bool GPop::SecretsImp::valid() const
{
	return m_secrets.valid() ;
}

std::string GPop::SecretsImp::secret(  const std::string & mechanism , const std::string & id ) const
{
	return m_secrets.secret( mechanism , id ) ;
}


