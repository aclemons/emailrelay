//
// Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gcounter.h
///

#ifndef G_OBJECT_COUNTER_H
#define G_OBJECT_COUNTER_H

#include "gdef.h"
#include "gdebug.h"

/// \namespace G
namespace G
{
	class CounterImp ;
}

/// \class G::CounterImp
/// A private implementation class used
/// by the G::Counter<> class template.
///
class G::CounterImp 
{
public: 
	static void check( const char * class_name , unsigned long n ) ;
		///< Checks the instance counter.
} ;

/// \namespace G
namespace G
{

/// \class G::Counter
/// An instance counter to help with leak testing.
///
/// Typically used as a private base class. (Strictly
/// this uses the curiously recurring template pattern, but 
/// it does not do any static_cast<D> downcasting.)
///
/// Note that the second template parameter (the class name)
/// needs a declaration like "extern char FooClassName[]", 
/// not a string literal.
///
template <typename D, const char * C>
class Counter : private CounterImp 
{
public:
	Counter() ;
		///< Constructor.

	~Counter() ;
		///< Destructor.

	Counter( const Counter<D,C> & ) ;
		///< Copy constructor.

	void operator=( const Counter<D,C> & ) ;
		///< Assignment operator.

private:
	static unsigned long m_n ;
} ;

template <typename D, const char * C>
unsigned long Counter<D,C>::m_n = 0UL ;

template <typename D, const char * C>
Counter<D,C>::Counter()
{
	m_n++ ;
	check( C , m_n ) ;
}

template <typename D, const char * C>
Counter<D,C>::~Counter()
{
	m_n-- ;
	check( C , m_n ) ;
}

template <typename D, const char * C>
Counter<D,C>::Counter( const Counter<D,C> & )
{
	m_n++ ;
	check( C , m_n ) ;
}

template <typename D, const char * C>
void Counter<D,C>::operator=( const Counter<D,C> & )
{
}

}

#endif
