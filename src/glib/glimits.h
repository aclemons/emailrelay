//
// Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file glimits.h
///

#ifndef G_LIMITS_H
#define G_LIMITS_H

#include "gdef.h"

namespace G
{
	class limits ;
}

/// \class G::limits
/// A scoping structure for a set of buffer sizes. Intended to be used to
/// reduce memory requirements in embedded environments.
///
class G::limits
{
public:

 #ifndef G_SMALL
	G_CONSTANT( int , log , 1000 ) ; // log line limit
	G_CONSTANT( int , file_buffer , 102400 ) ; // cf. BUFSIZ
	G_CONSTANT( int , file_slurp , 100000000 ) ;  // read file into contiguous memory
	G_CONSTANT( int , pipe_buffer , 4096 ) ; // one-off read from a pipe
	G_CONSTANT( int , net_buffer , 20000 ) ; // best if bigger than the TLS maximum block size of 16k
	G_CONSTANT( int , net_file_limit , 200000000 ) ; // d.o.s. network read file limit
	G_CONSTANT( int , net_listen_queue , 3 ) ;
 #else
	G_CONSTANT( int , log , 120 ) ;
	G_CONSTANT( int , file_buffer , 128 ) ;
	G_CONSTANT( int , file_slurp , 10000000 ) ;
	G_CONSTANT( int , pipe_buffer , 128 ) ;
	G_CONSTANT( int , net_buffer , 512 ) ;
	G_CONSTANT( int , net_file_limit , 10000000 ) ;
	G_CONSTANT( int , net_listen_queue , 3 ) ;
 #endif

private:
	limits() g__eq_delete ;
} ;

#endif
