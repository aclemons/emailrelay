//
// Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gfilterfactory.h
///

#ifndef G_SMTP_FILTER_FACTORY_H
#define G_SMTP_FILTER_FACTORY_H

#include "gdef.h"
#include "gfilter.h"
#include "gfactoryparser.h"
#include "gexceptionsink.h"
#include "gexception.h"
#include <string>
#include <utility>
#include <memory>

namespace GSmtp
{
	class FilterFactory ;
	class FilterFactoryFileStore ;
	class FileStore ;
}

//| \class GSmtp::FilterFactory
/// A factory interface for making GSmtp::Filter message processors.
///
class GSmtp::FilterFactory
{
public:
	virtual std::unique_ptr<Filter> newFilter( GNet::ExceptionSink ,
		bool server_side , const std::string & spec , unsigned int timeout ) = 0 ;
			///< Returns a Filter on the heap. The specification is
			///< normally prefixed with a processor type, or it is
			///< the file system path of a filter exectuable (see
			///< GSmtp::FactoryParser). Throws an exception if an
			///< invalid or unsupported specification.

	virtual ~FilterFactory() = default ;
		///< Destructor.
} ;

//| \class GSmtp::FilterFactoryFileStore
/// A filter factory that holds a GSmtp::FileStore reference so that
/// it can instantiate filters that operate on message files.
///
class GSmtp::FilterFactoryFileStore : public FilterFactory
{
public:
	explicit FilterFactoryFileStore( FileStore & ) ;
		///< Constructor. The FileStore reference is retained and passed
		///< to new filter objects so that they can derive the paths of
		///< the content and envelope files that they process.

	std::unique_ptr<Filter> newFilter( GNet::ExceptionSink ,
		bool server_side , const std::string & identifier , unsigned int timeout ) override ;

private:
	FileStore & m_file_store ;
} ;

#endif
