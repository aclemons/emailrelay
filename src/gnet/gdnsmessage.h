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
/// \file gdnsmessage.h
///

#ifndef G_NET_DNS_MESSAGE_H
#define G_NET_DNS_MESSAGE_H

#include "gdef.h"
#include "gexception.h"
#include "gaddress.h"
#include "gstringview.h"
#include <vector>
#include <utility>
#include <string>
#include <map>
#include <new>

namespace GNet
{
	class DnsMessage ;
	class DnsMessageRR ;
	class DnsMessageRecordType ;
	class DnsMessageQuestion ;
	class DnsMessageNameParser ;
	class DnsMessageRequest ;
	class DnsMessageRRData ;
	class DnsMessageDumper ;
	class DnsMessageBuilder ;
}

//| \class GNet::DnsMessage
/// A DNS message parser, with static factory functions for message composition.
/// A DnsMessage contains a Header and four sections: Question, Answer, Authority
/// and Additional. The Question section contains Question records (DnsMessageQuestion)
/// while the Answer, Authority and Additional sections contain RR records
/// (DnsMessageRR). Each RR has a standard header followed by RDATA.
/// \see RFC-1035
///
class GNet::DnsMessage
{
public:
	G_EXCEPTION( Error , tx("dns message error") ) ;
	using Question = DnsMessageQuestion ;
	using RR = DnsMessageRR ;

	explicit DnsMessage( const std::vector<char> & buffer ) ;
		///< Constructor. Check with valid().

	DnsMessage( const char * , std::size_t ) ;
		///< Constructor. Check with valid().

	bool valid() const ;
		///< Returns true if the message data is big enough
		///< for a header and its TC() flag is false.

	std::vector<Address> addresses() const ;
		///< Returns the Answer addresses.

	unsigned int ID() const ;
		///< Returns the header ID.

	bool QR() const ;
		///< Returns the header QR (query/response).

	unsigned int OPCODE() const ;
		///< Returns the header OPCODE.

	bool AA() const ;
		///< Returns the header AA flag (authorative).

	bool TC() const ;
		///< Returns the header TC flag (truncated).

	bool RD() const ;
		///< Returns the header RD (recursion desired).

	bool RA() const ;
		///< Returns the header RA (recursion available).

	unsigned int Z() const ;
		///< Returns the header Z value (zero).

	unsigned int RCODE() const ;
		///< Returns the header RCODE.

	unsigned int QDCOUNT() const ;
		///< Returns the header QDCOUNT field, ie. the number of
		///< records in the Question section.

	unsigned int ANCOUNT() const ;
		///< Returns the header ANCOUNT field, ie. the number of
		///< RR records in the Answer section.

	unsigned int NSCOUNT() const ;
		///< Returns the header NSCOUNT field, ie. the number of
		///< RR records in the Authority section.

	unsigned int ARCOUNT() const ;
		///< Returns the header ARCOUNT field, ie. the number of
		///< RR records in the Additional section.

	unsigned int recordCount() const ;
		///< Returns QDCOUNT()+ANCOUNT()+NSCOUNT()+ARCOUNT().

	Question question( unsigned int n ) const ;
		///< Returns the n'th record as a Question record.
		///< Precondition: n < QDCOUNT()

	RR rr( unsigned int n ) const ;
		///< Returns the n'th record as a RR record. The returned
		///< object retains a reference to this DnsMessage, so
		///< prefer rrAddress().
		///< Precondition: n >= QDCOUNT() && n < recordCount()

	Address rrAddress( unsigned int n ) const ;
		///< Returns the address in the n'th record.
		///< Throws if not A or AAAA.
		///< Precondition: n >= QDCOUNT()

	const char * p() const noexcept ;
		///< Returns the raw data.

	std::size_t n() const noexcept ;
		///< Returns the raw data size.

	unsigned int byte( unsigned int byte_index ) const ;
		///< Returns byte at the given offset.

	unsigned int word( unsigned int byte_index ) const ;
		///< Returns word at the given byte offset.

	std::string span( unsigned int begin , unsigned int end ) const ;
		///< Returns the data in the given half-open byte range.

	static DnsMessage request( const std::string & type , const std::string & hostname , unsigned int id = 0U ) ;
		///< Factory function for a request message of the give type
		///< ("A", "AAAA", etc). The type name is interpreted by
		///< DnsMessageRecordType::value().

	static DnsMessage rejection( const DnsMessage & request , unsigned int rcode ) ;
		///< Factory function for a failure response based on the given
		///< request message.

	static DnsMessage response( const DnsMessage & request , const Address & address ) ;
		///< Factory function for an answer response based on the given
		///< request message.

	static DnsMessage empty() ;
		///< Factory function for an unusable object. Most methods will
		///< throw, except n() will return zero.

private:
	friend class DnsMessageDumper ;
	DnsMessage() ;
	friend class DnsMessageBuilder ;
	void convertToResponse( unsigned int rcode , bool authoritative ) ;
	void addByte( unsigned int ) ;
	void addWord( unsigned int ) ;

private:
	std::vector<char> m_buffer ;
} ;

//| \class GNet::DnsMessageRecordType
/// A static class for mapping between a RR type name, such as "AAAA",
/// and its corresponding numeric value.
///
class GNet::DnsMessageRecordType
{
public:
	static unsigned int value( G::string_view type_name ) ;
		///< Returns the type value for the given type name.
		///< Throws on error.

	static unsigned int value( G::string_view type_name , std::nothrow_t ) noexcept ;
		///< Returns the type value for the given type name,
		///< or zero on error.

	static std::string name( unsigned int type_value ) ;
		///< Returns the type name for the given type value.

public:
	DnsMessageRecordType() = delete ;
} ;

//| \class GNet::DnsMessageRRData
/// A trivial mix-in base class that simplifies method names
/// when accessing data from a DnsMessageRR derived class.
///
class GNet::DnsMessageRRData
{
public:
	unsigned int byte( unsigned int offset ) const ;
		///< Calls rdataByte().

	unsigned int word( unsigned int offset ) const ;
		///< Calls rdataWord().

	std::string span( unsigned int begin , unsigned int end ) const ;
		///< Calls rdataSpan().

	std::string span( unsigned int begin ) const ;
		///< Calls rdataSpan().

	std::string dname( unsigned int rdata_offset ) const ;
		///< Calls rdataDname().

	std::string dname( unsigned int * rdata_offset_inout_p ) const ;
		///< Calls rdataDname().

	unsigned int offset() const ;
		///< Calls rdataOffset().

	unsigned int size() const ;
		///< Calls rdataSize().

protected:
	DnsMessageRRData() = default ;
} ;

//| \class GNet::DnsMessageRR
/// Represents DNS response record.
///
class GNet::DnsMessageRR : private DnsMessageRRData
{
public:
	using RR = DnsMessageRR ;

public:
	DnsMessageRR( const DnsMessage & , unsigned int offset ) ;
		///< Constructor from DnsMessage data. Keeps the
		///< DnsMessage reference, which is then passed
		///< to copies.

	bool isa( G::string_view ) const noexcept ;
		///< Returns true if the type() has the given name().

	unsigned int type() const ;
		///< Returns the RR TYPE value().

	unsigned int class_() const ;
		///< Returns the RR CLASS value().

	unsigned int size() const ;
		///< Returns the size of the RR.

	std::string name() const ;
		///< Returns the RR NAME.

	Address address( unsigned int port = 0U ) const ;
		///< Returns the Address if isa(A) or isa(AAAA).
		///< Throws if not A or AAAA.

	Address address( unsigned int port , std::nothrow_t ) const ;
		///< Returns the Address if isa(A) or isa(AAAA).
		///< Returns Address::defaultAddress() (with a zero
		///< port number) if not valid.

	const DnsMessageRRData & rdata() const ;
		///< Provides access to the message RDATA.

private:
	friend class GNet::DnsMessageRRData ;
	std::string rdataDname( unsigned int rdata_offset ) const ;
	std::string rdataDname( unsigned int * rdata_offset_inout_p ) const ;
	std::string rdataSpan( unsigned int begin ) const ;
	std::string rdataSpan( unsigned int begin , unsigned int end ) const ;
	unsigned int rdataOffset() const ;
	unsigned int rdataSize() const ;
	unsigned int rdataByte( unsigned int offset ) const ;
	unsigned int rdataWord( unsigned int offset ) const ;
	GNet::Address addressImp( unsigned int port , bool & ok ) const ;

private:
	const DnsMessage & m_msg ;
	unsigned int m_offset {0U} ;
	unsigned int m_size {0U} ;
	unsigned int m_type {0U} ;
	unsigned int m_class {0U} ;
	unsigned int m_rdata_offset {0U} ;
	unsigned int m_rdata_size {0U} ;
	std::string m_name ;
} ;

//| \class GNet::DnsMessageQuestion
/// Represents DNS question record.
///
class GNet::DnsMessageQuestion
{
public:
	DnsMessageQuestion( const DnsMessage & , unsigned int offset ) ;
		///< Constructor.

	unsigned int size() const ;
		///< Returns the record size.

	unsigned int qtype() const ;
		///< Returns the question QTYPE value.

	unsigned int qclass() const ;
		///< Returns the question QCLASS value.

	std::string qname() const ;
		///< Returns the question domain name (QNAME).

private:
	unsigned int m_size {0U} ;
	unsigned int m_qtype {0U} ;
	unsigned int m_qclass {0U} ;
	std::string m_qname ;
} ;

//| \class GNet::DnsMessageNameParser
/// An implementation class used by GNet::DnsMessage to parse
/// compressed domain names.
///
class GNet::DnsMessageNameParser
{
public:
	static unsigned int size( const DnsMessage & msg , unsigned int ) ;
		///< Returns the size of the compressed name at the given offset.

	static std::string read( const DnsMessage & msg , unsigned int ) ;
		///< Returns the decompressed domain name at the given offset,
		///< made up of the labels with dots inbetween.

public:
	DnsMessageNameParser() = delete ;
} ;

//| \class GNet::DnsMessageRequest
/// Represents a DNS query message.
///
class GNet::DnsMessageRequest
{
public:
	using RR = DnsMessageRR ;

	DnsMessageRequest( const std::string & type , const std::string & hostname , unsigned int id = 0U ) ;
		///< Constructor.

	const char * p() const ;
		///< Returns a pointer to the message data.

	std::size_t n() const ;
		///< Returns message size.

private:
	void addDomainName( const std::string & domain , char sep ) ;
	void addLabel( G::string_view ) ;
	void addWord( unsigned int ) ;
	void addByte( unsigned int ) ;

private:
	std::string m_data ;
} ;

inline
const GNet::DnsMessageRRData & GNet::DnsMessageRR::rdata() const
{
	return *this ;
}

inline
std::string GNet::DnsMessageRRData::dname( unsigned int offset ) const
{
	return static_cast<const DnsMessageRR *>(this)->rdataDname( offset ) ;
}

inline
std::string GNet::DnsMessageRRData::dname( unsigned int * offset_inout_p ) const
{
	return static_cast<const DnsMessageRR *>(this)->rdataDname( offset_inout_p ) ;
}

inline
std::string GNet::DnsMessageRRData::span( unsigned int begin ) const
{
	return static_cast<const DnsMessageRR *>(this)->rdataSpan( begin ) ;
}

inline
std::string GNet::DnsMessageRRData::span( unsigned int begin , unsigned int end ) const
{
	return static_cast<const DnsMessageRR *>(this)->rdataSpan( begin , end ) ;
}

inline
unsigned int GNet::DnsMessageRRData::size() const
{
	return static_cast<const DnsMessageRR *>(this)->rdataSize() ;
}

inline
unsigned int GNet::DnsMessageRRData::byte( unsigned int offset ) const
{
	return static_cast<const DnsMessageRR *>(this)->rdataByte( offset ) ;
}

inline
unsigned int GNet::DnsMessageRRData::word( unsigned int offset ) const
{
	return static_cast<const DnsMessageRR *>(this)->rdataWord( offset ) ;
}

inline
unsigned int GNet::DnsMessage::recordCount() const
{
	return QDCOUNT() + ANCOUNT() + NSCOUNT() + ARCOUNT() ;
}

#endif
