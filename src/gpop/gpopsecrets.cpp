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
// gpopsecrets.cpp
//

#include "gdef.h"
#include "gpop.h"
#include "gpopsecrets.h"
#include "gsecrets.h" // gsmtp

/// \class GPop::SecretsImp
/// A private pimple-pattern implementation class used by GPop::Secrets.
///  The implementation delegates to GSmtp::Secrets.
/// 
class GPop::SecretsImp 
{
public:
	explicit SecretsImp( const std::string & path ) ;
	std::string path() const ;
	std::string secret( const std::string & mechanism , const std::string & id ) const ;
	bool contains( const std::string & mechanism ) const ;
	std::string m_path ;
	GAuth::Secrets m_secrets ;
} ;

// ===

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

std::string GPop::Secrets::source() const
{
	return m_imp->path() ;
}

bool GPop::Secrets::valid() const
{
	return true ;
}

std::string GPop::Secrets::secret( const std::string & mechanism , const std::string & id ) const
{
	return m_imp->secret( mechanism , id ) ;
}

bool GPop::Secrets::contains( const std::string & mechanism ) const
{
	return m_imp->contains( mechanism ) ;
}

// ===

GPop::SecretsImp::SecretsImp( const std::string & path ) :
	m_path(path) ,
	m_secrets( path , "pop-server" ) // throws on error
{
	// throw if an empty path
	if( !m_secrets.valid() )
		throw GPop::Secrets::OpenError(path) ;
}

std::string GPop::SecretsImp::path() const
{
	return m_path ;
}

std::string GPop::SecretsImp::secret( const std::string & mechanism , const std::string & id ) const
{
	return m_secrets.secret( mechanism , id ) ;
}

bool GPop::SecretsImp::contains( const std::string & mechanism ) const
{
	return m_secrets.contains( mechanism ) ;
}

