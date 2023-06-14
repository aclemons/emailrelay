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
/// \file gassert.h
///

#ifndef G_ASSERT_H
#define G_ASSERT_H

#include "gdef.h"
#include "glogoutput.h"

#if defined(G_WITH_ASSERT) || ( defined(_DEBUG) && ! defined(G_NO_ASSERT) )
	#define G_ASSERT( test ) G::LogOutput::assertion( __FILE__ , __LINE__ , (test) , #test )
#else
	#define G_ASSERT( test )
#endif

#endif
