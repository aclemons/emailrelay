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
/// \file gsecrets.cpp
///

#include "gdef.h"
#include "gsecrets.h"
#include "gsecretsfile.h"
#include "gbase64.h"
#include "gassert.h"
#include "glog.h"

namespace GAuth
{
	namespace SecretsImp
	{
		bool pam( const std::string & s )
		{
			return !G::is_windows() && ( s == "pam:" || s == "/pam" ) ;
		}
		bool plain( const std::string & s )
		{
			return G::Str::headMatch( s , "plain:" ) ;
		}
		bool parse( const std::string & s , std::string & id , std::string & pwd )
		{
			std::string spec = G::Str::tail( s , ":" ) ;
			id = G::Str::head( spec , ":" , false ) ;
			pwd = G::Str::tail( spec , ":" , true ) ;
			return G::Base64::valid(id) && G::Base64::valid(pwd) ;
		}
		void check( const std::string & s )
		{
			if( plain(s) ) // account on the command-line, no secrets file
			{
				std::string id ;
				std::string pwd ;
				if( !parse( s , id , pwd ) )
					throw Secrets::ClientAccountError() ;
			}
			else
			{
				SecretsFile::check( s , true ) ;
			}
		}
	}
}

void GAuth::Secrets::check( const std::string & c , const std::string & s , const std::string & p )
{
	namespace imp = SecretsImp ;
	if( !c.empty() ) imp::check( c ) ;
	if( !s.empty() && !imp::pam(s) && s != c ) SecretsFile::check( s , true ) ;
	if( !p.empty() && !imp::pam(p) && p != s && p != c ) SecretsFile::check( p , true ) ;
}

std::unique_ptr<GAuth::SaslServerSecrets> GAuth::Secrets::newServerSecrets( const std::string & path ,
	const std::string & log_name )
{
	return std::make_unique<SecretsFileServer>( path , log_name ) ;
}

std::unique_ptr<GAuth::SaslClientSecrets> GAuth::Secrets::newClientSecrets( const std::string & path ,
	const std::string & log_name )
{
	return std::make_unique<SecretsFileClient>( path , log_name ) ;
}

// ==

GAuth::SecretsFileClient::SecretsFileClient( const std::string & path , const std::string & log_name ) :
	m_id_pwd(SecretsImp::plain(path)) ,
	m_file(m_id_pwd?std::string():path,true,log_name)
{
	if( m_id_pwd )
		SecretsImp::parse( path , m_id , m_pwd ) ;
}

GAuth::SecretsFileClient::~SecretsFileClient()
= default ;

bool GAuth::SecretsFileClient::validSelector( G::string_view selector ) const
{
	if( m_id_pwd )
		return selector.empty() ;
	else if( !m_file.valid() )
		return selector.empty() ;
	else
		return m_file.containsClientSelector( selector ) ;
}

bool GAuth::SecretsFileClient::mustAuthenticate( G::string_view selector ) const
{
	if( m_id_pwd )
		return true ;
	else if( !m_file.valid() )
		return false ;
	else
		return m_file.containsClientSecret( selector ) ;
}

GAuth::Secret GAuth::SecretsFileClient::clientSecret( G::string_view type , G::string_view selector ) const
{
	if( m_id_pwd && type == "plain"_sv )
	{
		return { {m_id,"base64"} , {m_pwd,"base64"} } ;
	}
	else if( m_id_pwd )
	{
		return GAuth::Secret::none() ;
	}
	else
	{
		return m_file.clientSecret( type , selector ) ;
	}
}

// ==

GAuth::SecretsFileServer::SecretsFileServer( const std::string & spec , const std::string & log_name ) :
	m_pam(SecretsImp::pam(spec)) ,
	m_file(m_pam?std::string():spec,true,log_name)
{
}

GAuth::SecretsFileServer::~SecretsFileServer()
= default ;

std::string GAuth::SecretsFileServer::source() const
{
	return m_pam ? std::string("pam:") : m_file.path() ;
}

bool GAuth::SecretsFileServer::valid() const
{
	return m_pam || m_file.valid() ;
}

GAuth::Secret GAuth::SecretsFileServer::serverSecret( G::string_view type , G::string_view id ) const
{
	G_ASSERT( !m_pam ) ;
	return m_file.serverSecret( type , id ) ;
}

std::pair<std::string,std::string> GAuth::SecretsFileServer::serverTrust( const std::string & address_range ) const
{
	G_ASSERT( !m_pam ) ;
	return m_file.serverTrust( address_range ) ;
}

bool GAuth::SecretsFileServer::contains( G::string_view type , G::string_view id ) const
{
	G_ASSERT( !m_pam ) ;
	return m_file.containsServerSecret( type , id ) ;
}

