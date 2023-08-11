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
/// \file grandom.cpp
///

#include "gdef.h"
#include "grandom.h"
#include "gprocess.h"
#include <ctime>
#include <chrono>
#include <random>

unsigned int G::Random::rand( unsigned int start , unsigned int end )
{
	static std::default_random_engine e ; // NOLINT cert-msc32-c

	static bool seeded = false ;
	if( !seeded )
	{
		#if defined(G_WINDOWS)
			std::random_device r ;
		#else
			std::random_device r( "/dev/urandom" ) ;
		#endif

		using seed_t = std::random_device::result_type ;
		seed_t seed_1 = 0U ;
		try { seed_1 = r() ; } catch( std::exception & ) {}

		auto tp = std::chrono::high_resolution_clock::now() ;
		auto seed_2 = static_cast<seed_t>( tp.time_since_epoch().count() ) ;

		seed_t seed_3 = G::Process::Id().seed<seed_t>() ;

		std::seed_seq seq{ seed_1 , seed_2 , seed_3 } ;
		e.seed( seq ) ;
		seeded = true ;
	}

	std::uniform_int_distribution<unsigned int> dist( start , end ) ;
	return dist( e ) ;
}

