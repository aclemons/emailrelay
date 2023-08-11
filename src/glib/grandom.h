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
/// \file grandom.h
///

#ifndef G_RANDOM_H
#define G_RANDOM_H

#include "gdef.h"
#include <random>

namespace G
{
	namespace Random /// An enclosing namespace for G::Random::rand().
	{
		unsigned int rand( unsigned int start = 0U , unsigned int end = 32767 ) ;
			///< Returns a random value, uniformly distributed over the
			///< given range (including 'start' and 'end'), and automatically
			///< seeded on first use.
	}
}

#endif
