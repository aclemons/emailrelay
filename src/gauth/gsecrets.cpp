//
// Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gsecrets.cpp
///

#include "gdef.h"
#include "gsecrets.h"
#include "gsecretsfile.h"
#include "glog.h"
#include <algorithm>

void GAuth::Secrets::check( const std::string & p1 , const std::string & p2 , const std::string & p3 )
{
	G::StringArray list { p1 , p2 , p3 } ;
	list.erase( std::remove(list.begin(),list.end(),std::string()) , list.end() ) ;
	list.erase( std::remove(list.begin(),list.end(),"/pam") , list.end() ) ;
	std::sort( list.begin() , list.end() ) ;
	list.erase( std::unique(list.begin(),list.end()) , list.end() ) ;
	std::for_each( list.begin() , list.end() , &SecretsFile::check ) ;
}

GAuth::Secrets::Secrets( const std::string & path , const std::string & log_name ) :
	m_source(path)
{
	G_DEBUG( "GAuth::Secrets:ctor: [" << path << "]" ) ;
	if( m_source != "/pam" )
		m_imp = std::make_unique<SecretsFile>( path , true , log_name ) ;
}

GAuth::Secrets::Secrets()
{
	if( m_source != "/pam" )
		m_imp = std::make_unique<SecretsFile>( std::string() , false , std::string() ) ;
}

GAuth::Secrets::~Secrets()
= default ;

std::string GAuth::Secrets::source() const
{
	return m_source ;
}

bool GAuth::Secrets::valid() const
{
	return m_source == "/pam" || m_imp->valid() ;
}

GAuth::Secret GAuth::Secrets::clientSecret( const std::string & type ) const
{
	return valid() ? m_imp->clientSecret(type) : Secret::none() ;
}

GAuth::Secret GAuth::Secrets::serverSecret( const std::string & type , const std::string & id ) const
{
	return valid() ? m_imp->serverSecret( type , id ) : Secret::none() ;
}

std::pair<std::string,std::string> GAuth::Secrets::serverTrust( const std::string & address_range ) const
{
	return valid() ? m_imp->serverTrust( address_range ) : std::make_pair(std::string(),std::string()) ;
}

bool GAuth::Secrets::contains( const std::string & type , const std::string & id ) const
{
	return valid() ? m_imp->contains( type , id ) : false ;
}

