//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gssl_use_both.cpp
///

#include "gdef.h"
#include "gssl.h"
#include "gssl_openssl.h"
#include "gssl_mbedtls.h"
#include "gtest.h"

std::unique_ptr<GSsl::LibraryImpBase> GSsl::Library::newLibraryImp( G::StringArray & library_config , Library::LogFn log_fn , bool verbose )
{
	if( LibraryImpBase::consume(library_config,"mbedtls") || G::Test::enabled("ssl-use-mbedtls") )
	{
		return std::make_unique<MbedTls::LibraryImp>( library_config , log_fn , verbose ) ;
	}
	else
	{
		LibraryImpBase::consume( library_config , "openssl" ) ;
		return std::make_unique<OpenSSL::LibraryImp>( library_config , log_fn , verbose ) ;
	}
}

std::string GSsl::Library::credit( const std::string & prefix , const std::string & eol , const std::string & eot )
{
	return
		OpenSSL::LibraryImp::credit( prefix , eol , eol ) +
		MbedTls::LibraryImp::credit( prefix , eol , eot ) ;
}

std::string GSsl::Library::ids()
{
	return OpenSSL::LibraryImp::sid() + ", " + MbedTls::LibraryImp::sid() ;
}

