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
/// \file submission.h
///

#ifndef G_MAIN_SUBMISSION_H
#define G_MAIN_SUBMISSION_H

#include "gdef.h"
#include "garg.h"

namespace Main
{
	class Submission ;
}

//| \class Main::Submission
/// Does simple message submission from the command-line.
///
class Main::Submission
{
public:
	static bool enabled( const G::Arg & ) ;
		///< Returns true if the submit functionality is enabled by
		///< the build and argv[0].

	static int submit( const G::Arg & ) ;
		///< Does message submission. Returns an exit code.
} ;

#endif
