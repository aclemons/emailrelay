//
// Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gpamerror.h
///

#ifndef G_PAMERROR_H
#define G_PAMERROR_H

#include "gdef.h"
#include "gexception.h"
#include <string>

namespace G
{
	class PamError ;
}

/// \class G::PamError
/// An exception class used by G::Pam.
///
class G::PamError : public G::Exception
{
public:
	PamError( const std::string & op , int pam_error ) ;
		///< Constructor.

	PamError( const std::string & op , int pam_error , const char * ) ;
		///< Constructor.
} ;

#endif
