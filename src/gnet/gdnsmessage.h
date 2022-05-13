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
/// \file gdnsmessage.h
///

#ifndef G_NET_DNS_MESSAGE_H
#define G_NET_DNS_MESSAGE_H

#include "gdef.h"
#include "gexception.h"
#include "gaddress.h"
#include <vector>
#include <utility>
#include <string>
#include <map>

namespace GNet
{
	class DnsMessage ;
	class DnsMessageRR ;
	class DnsMessageRecordType ;
	class DnsMessageQuestion ;
	class DnsMessageNameParser ;
	class DnsMessageRequest ;
	class DnsMessageDumper ;
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
		///< Constructor.

	DnsMessage( const char * , std::size_t ) ;
		///< Constructor.

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

	Question question( unsigned int n ) const ;
		///< Returns the n'th record as a Question record.
		///< Precondition: n < QDCOUNT()

	RR rr( unsigned int n ) const ;
		///< Returns the n'th record as a RR record. The returned
		///< object retains a reference to this DnsMessage, so
		///< prefer rrAddress().
		///< Precondition: n >= QDCOUNT() && n < (QDCOUNT()+ANCOUNT()+NSCOUNT()+ARCOUNT())

	Address rrAddress( unsigned int n ) const ;
		///< Returns the address in the n'th record treated as a RR record.

	const char * p() const ;
		///< Returns the raw data.

	std::size_t n() const ;
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

	static DnsMessage empty() ;
		///< Factory function for an unusable object. Most methods will
		///< throw, except n() will return zero.

private:
	friend class DnsMessageDumper ;
	DnsMessage() ;
	void reject( unsigned int rcode ) ;

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
	static unsigned int value( const std::string & type_name ) ;
		///< Returns the type value for the given type name.

	static std::string name( unsigned int type_value ) ;
		///< Returns the type name for the given type value.

public:
	DnsMessageRecordType() = delete ;
} ;

//| \class GNet::DnsMessageRR
/// Represents DNS response record.
///
class GNet::DnsMessageRR
{
public:
	using RR = DnsMessageRR ;

public:
	DnsMessageRR( const DnsMessage & , unsigned int offset ) ;
		///< Constructor. Keeps the reference, which is then passed
		///< to copies.

	bool isa( const std::string & ) const ;
		///< Returns true if the type() has the given name().

	unsigned int type() const ;
		///< Returns the type value().

	unsigned int size() const ;
		///< Returns the size.

	std::string name() const ;
		///< Returns the NAME.

	Address address() const ;
		///< Returns the Address if isa(A) or isa(AAAA).

private:
	friend class DnsMessageDumper ;
	std::string rdata_dname( unsigned int rdata_offset ) const ;
	std::string rdata_dname( unsigned int * rdata_offset_p ) const ;
	std::string rdata_span( unsigned int begin ) const ;
	std::string rdata_span( unsigned int begin , unsigned int end ) const ;
	unsigned int rdata_offset() const ;
	unsigned int rdata_size() const ;
	unsigned int rdata_byte( unsigned int offset ) const ;
	unsigned int rdata_word( unsigned int offset ) const ;

private:
	const DnsMessage & m_msg ;
	unsigned int m_offset ;
	unsigned int m_size ;
	unsigned int m_type ;
	unsigned int m_class ;
	unsigned int m_rdata_offset ;
	unsigned int m_rdata_size ;
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

	std::string qname() const ;
		///< Returns the subject of the question.

private:
	unsigned int m_size ;
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
		///< Returns the size of the compressed name.

	static std::string read( const DnsMessage & msg , unsigned int ) ;
		///< Returns the decompressed name, made up of the
		///< labels with dots inbetween.

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
	void q( const std::string & domain , char ) ;
	void q( const std::string & ) ;
	void q( unsigned int ) ;
	void q( int ) ;

private:
	std::string m_data ;
} ;

#endif
