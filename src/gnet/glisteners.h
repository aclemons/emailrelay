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
/// \file glisteners.h
///

#ifndef G_LISTENERS_H
#define G_LISTENERS_H

#include "gdef.h"
#include "ginterfaces.h"
#include "gaddress.h"
#include "gstringarray.h"
#include "gexception.h"
#include <vector>
#include <string>

namespace GNet
{
	class Listeners ;
}

//| \class GNet::Listeners
/// Represents a set of listening inputs which can be file-descriptor,
/// interface or network address.
///
class GNet::Listeners
{
public:
	G_EXCEPTION( InvalidFd , tx("invalid listening file descriptor number") )

	Listeners( const Interfaces & , const G::StringArray & listener_spec_list , unsigned int port ) ;
		///< Constructor. The specification strings can be like "fd#3"
		///< for a file descriptor, "127.0.0.1" for a fixed address,
		///< or "ppp0-ipv4" for an interface. If the specification
		///< list is empty then the two fixed wildcard addresses
		///< are added.

	bool defunct() const ;
		///< Returns true if no inputs and static.

	bool idle() const ;
		///< Returns true if no inputs but some interfaces might come up.

	bool hasBad() const ;
		///< Returns true if one or more inputs are invalid.

	std::string badName() const ;
		///< Returns the first invalid input.

	bool hasEmpties() const ;
		///< Returns true if some named interfaces have no addresses.

	std::string logEmpties() const ;
		///< Returns a log-line snippet for hasEmpties().

	bool noUpdates() const ;
		///< Returns true if some inputs are interfaces but
		///< GNet::Interfaces is not active().

	const std::vector<int> & fds() const ;
		///< Exposes the list of fd inputs.

	const std::vector<Address> & fixed() const ;
		///< Exposes the list of address inputs.

	const std::vector<Address> & dynamic() const ;
		///< Exposes the list of interface addresses.

private:
	bool empty() const ;
	void addWildcards( unsigned int ) ;
	static int parseFd( const std::string & ) ;
	static bool isAddress( const std::string & , unsigned int ) ;
	static Address address( const std::string & , unsigned int ) ;
	static int af( const std::string & ) ;
	static std::string basename( const std::string & ) ;
	static bool isBad( const std::string & ) ;

private:
	std::string m_bad ;
	G::StringArray m_empties ;
	G::StringArray m_used ;
	std::vector<Address> m_fixed ;
	std::vector<Address> m_dynamic ;
	std::vector<int> m_fds ;
} ;

#endif
