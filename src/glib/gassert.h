//
// Copyright (C) 2001-2006 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gassert.h
//

#ifndef G_ASSERT_H
#define G_ASSERT_H

#include "gdef.h"
#include "glogoutput.h"

#if defined(_DEBUG) && ! defined(G_NO_ASSERT)
	#define G_ASSERT( test ) G::LogOutput::assertion( __FILE__ , __LINE__ , test , #test )
#else
	#define G_ASSERT( test )
#endif

#endif
