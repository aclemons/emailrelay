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
/// \file gdnsmessage.cpp
///

#include "gdef.h"
#include "gdnsmessage.h"
#include "gassert.h"
#include "gstr.h"
#include "gstringview.h"
#include "gstringfield.h"
#include <array>
#include <vector>
#include <iomanip>
#include <sstream>
#include <stdexcept>

GNet::DnsMessageRequest::DnsMessageRequest( const std::string & type , const std::string & hostname , unsigned int id )
{
	// header section
	G_ASSERT( id < 0xffffU ) ;
	q( (id>>8U)&0xff ) ; q( id&0xff ) ; // ID=id - arbitrary identifier to link query with response
	q( 0x01 ) ; // flags - QR=0 (ie. query) and RD=1 (ie. recursion desired)
	q( 0x00 ) ; // RA=0 (recursion available) and Z=0 (zero bits, but see RFC-2671) and RCODE=0 (response code)
	q( 0x00 ) ; q( 0x01 ) ; // QDCOUNT=1 (ie. one question section)
	q( 0x00 ) ; q( 0x00 ) ; // ANCOUNT=0 (ie. no answer sections)
	q( 0x00 ) ; q( 0x00 ) ; // NSCOUNT=0 (ie. no authority sections)
	q( 0x00 ) ; q( 0x00 ) ; // ARCOUNT=0 (ie. no additional sections)

	// question section
	q( hostname , '.' ) ; // QNAME
	q( 0x00 ) ; q( DnsMessageRecordType::value(type) ) ; // eg. QTYPE=A
	q( 0x00 ) ; q( 0x01 ) ; // QCLASS=IN(ternet)
}

void GNet::DnsMessageRequest::q( const std::string & domain , char sep )
{
	G::string_view domain_sv( domain ) ;
	for( G::StringFieldView part( domain_sv , sep ) ; part ; ++part )
	{
		q( part() ) ;
	}
	q( G::string_view() ) ;
}

void GNet::DnsMessageRequest::q( G::string_view data )
{
	if( data.size() > 63U ) throw DnsMessage::Error("overflow") ;
	q( static_cast<unsigned int>(data.size()) ) ;
	if( !data.empty() )
		m_data.append( data.data() , data.size() ) ;
}

void GNet::DnsMessageRequest::q( int n )
{
	q( static_cast<unsigned int>(n) ) ;
}

void GNet::DnsMessageRequest::q( unsigned int n )
{
	m_data.append( 1U , static_cast<char>(n) ) ;
}

const char * GNet::DnsMessageRequest::p() const
{
	return m_data.data() ;
}

std::size_t GNet::DnsMessageRequest::n() const
{
	return m_data.size() ;
}

// ==

GNet::DnsMessage::DnsMessage()
= default;

GNet::DnsMessage::DnsMessage( const std::vector<char> & buffer ) :
	m_buffer(buffer)
{
}

GNet::DnsMessage::DnsMessage( const char * p , std::size_t n ) :
	m_buffer(p,p+n)
{
}

bool GNet::DnsMessage::valid() const
{
	return m_buffer.size() >= 12U && !TC() ;
}

const char * GNet::DnsMessage::p() const noexcept
{
	return &m_buffer[0] ;
}

std::size_t GNet::DnsMessage::n() const noexcept
{
	return m_buffer.size() ;
}

#ifndef G_LIB_SMALL
GNet::DnsMessage GNet::DnsMessage::empty()
{
	return DnsMessage() ;
}
#endif

GNet::DnsMessage GNet::DnsMessage::request( const std::string & type , const std::string & hostname , unsigned int id )
{
	DnsMessageRequest r( type , hostname , id ) ;
	return DnsMessage( r.p() , r.n() ) ;
}

std::vector<GNet::Address> GNet::DnsMessage::addresses() const
{
	std::vector<Address> list ;
	for( unsigned int i = QDCOUNT() ; i < (QDCOUNT()+ANCOUNT()) ; i++ )
	{
		list.push_back( rrAddress(i) ) ;
	}
	return list ;
}

unsigned int GNet::DnsMessage::byte( unsigned int i ) const
{
	char c = m_buffer.at(i) ;
	return static_cast<unsigned char>(c) ;
}

unsigned int GNet::DnsMessage::word( unsigned int i ) const
{
	return byte(i) * 256U + byte(i+1U) ;
}

std::string GNet::DnsMessage::span( unsigned int begin , unsigned int end ) const
{
	if( begin >= m_buffer.size() || end > m_buffer.size() || begin > end )
		throw Error( "span error" ) ;
	return std::string( m_buffer.begin()+begin , m_buffer.begin()+end ) ;
}

unsigned int GNet::DnsMessage::ID() const
{
	return word( 0U ) ;
}

bool GNet::DnsMessage::QR() const
{
	return !!( byte(2U) & 0x80 ) ;
}

#ifndef G_LIB_SMALL
unsigned int GNet::DnsMessage::OPCODE() const
{
	return ( byte(2U) & 0x78 ) >> 3U ;
}
#endif

bool GNet::DnsMessage::AA() const
{
	return !!( byte(2U) & 0x04 ) ;
}

bool GNet::DnsMessage::TC() const
{
	return !!( byte(2U) & 0x02 ) ;
}

#ifndef G_LIB_SMALL
bool GNet::DnsMessage::RD() const
{
	return !!( byte(2U) & 0x01 ) ;
}
#endif

#ifndef G_LIB_SMALL
bool GNet::DnsMessage::RA() const
{
	return !!( byte(3U) & 0x80 ) ;
}
#endif

#ifndef G_LIB_SMALL
unsigned int GNet::DnsMessage::Z() const
{
	return ( byte(3U) & 0x70 ) >> 4 ;
}
#endif

unsigned int GNet::DnsMessage::RCODE() const
{
	return byte(3U) & 0x0f ;
}

unsigned int GNet::DnsMessage::QDCOUNT() const
{
	return word(4U) ;
}

unsigned int GNet::DnsMessage::ANCOUNT() const
{
	return word(6U) ;
}

#ifndef G_LIB_SMALL
unsigned int GNet::DnsMessage::NSCOUNT() const
{
	return word(8U) ;
}
#endif

#ifndef G_LIB_SMALL
unsigned int GNet::DnsMessage::ARCOUNT() const
{
	return word(10U) ;
}
#endif

#ifndef G_LIB_SMALL
GNet::DnsMessage GNet::DnsMessage::rejection( const DnsMessage & message , unsigned int rcode )
{
	DnsMessage result( message ) ;
	result.reject( rcode ) ;
	return result ;
}
#endif

void GNet::DnsMessage::reject( unsigned int rcode )
{
	if( m_buffer.size() < 10U )
		throw std::out_of_range( "dns message buffer too small" ) ;

	unsigned char * buffer = reinterpret_cast<unsigned char*>(&m_buffer[0]) ;
	buffer[2U] |= 0x80U ; // QR
	buffer[3U] &= 0xf0U ; buffer[3U] |= ( rcode & 0x0fU ) ; // RCODE
	buffer[6U] = 0U ; buffer[7U] = 0U ; // ANCOUNT
	buffer[8U] = 0U ; buffer[9U] = 0U ; // NSCOUNT

	// chop off RRs
	unsigned int new_size = 12U ; // HEADER size
	for( unsigned int i = 0U ; i < QDCOUNT() ; i++ )
		new_size += Question(*this,new_size).size() ;
	m_buffer.resize( new_size ) ;
}

#ifndef G_LIB_SMALL
GNet::DnsMessageQuestion GNet::DnsMessage::question( unsigned int record_index ) const
{
	if( record_index >= QDCOUNT() ) throw Error( "invalid record number" ) ;
	unsigned int offset = 12U ; // HEADER size
	for( unsigned int i = 0U ; i < record_index ; i++ )
		offset += Question(*this,offset).size() ;
	return Question(*this,offset) ;
}
#endif

GNet::DnsMessageRR GNet::DnsMessage::rr( unsigned int record_index ) const
{
	if( record_index < QDCOUNT() ) throw Error( "invalid rr number" ) ;
	unsigned int offset = 12U ; // HEADER size
	for( unsigned int i = 0U ; i < record_index ; i++ )
	{
		if( i < QDCOUNT() )
			offset += Question(*this,offset).size() ;
		else
			offset += RR(*this,offset).size() ;
	}
	return RR( *this , offset ) ;
}

GNet::Address GNet::DnsMessage::rrAddress( unsigned int record_index ) const
{
	return rr(record_index).address() ;
}

// ==

GNet::DnsMessageQuestion::DnsMessageQuestion( const DnsMessage & msg , unsigned int offset ) :
	m_size(0U)
{
	m_qname = DnsMessageNameParser::read( msg , offset ) ;
	m_size = DnsMessageNameParser::size( msg , offset ) + 2U + 2U ; // QNAME + QTYPE + QCLASS
}

unsigned int GNet::DnsMessageQuestion::size() const
{
	return m_size ;
}

#ifndef G_LIB_SMALL
std::string GNet::DnsMessageQuestion::qname() const
{
	return m_qname ;
}
#endif

// ==

unsigned int GNet::DnsMessageNameParser::size( const DnsMessage & msg , unsigned int offset_in )
{
	unsigned int offset = offset_in ;
	for(;;)
	{
		unsigned int n = msg.byte( offset ) ;
		if( ( n & 0xC0 ) == 0xC0 ) // compression -- see RFC-1035 4.1.4
			return offset - offset_in + 2U ;
		else if( ( n & 0xC0 ) != 0 )
			throw DnsMessage::Error( "unknown label type" ) ; // "reserved for future use"
		else if( n == 0U )
			break ;
		else
			offset += (n+1U) ;
	}
	return offset - offset_in + 1U ;
}

std::string GNet::DnsMessageNameParser::read( const DnsMessage & msg , unsigned int offset_in )
{
	unsigned int offset = offset_in ;
	std::string result ;
	for(;;)
	{
		unsigned int n = msg.byte( offset ) ;
		if( ( n & 0xC0 ) == 0xC0 )
		{
			unsigned int m = msg.byte(offset+1U) ;
			offset = (n&0x3F)*256U + m ;
		}
		else if( ( n & 0xC0 ) != 0 )
		{
			throw DnsMessage::Error( "unknown label type" ) ; // "reserved for future use"
		}
		else if( n == 0U )
		{
			break ;
		}
		else
		{
			if( n > 63U )
				throw DnsMessage::Error( "name overflow" ) ;

			result.append( result.empty()?0U:1U , '.' ) ;
			result.append( msg.span(offset+1U,offset+n+1U) ) ;
			offset += (n+1U) ;
		}
	}
	return result ;
}

// ==

GNet::DnsMessageRR::DnsMessageRR( const DnsMessage & msg , unsigned int offset ) :
	m_msg(msg) ,
	m_offset(offset) ,
	m_size(0U) ,
	m_type(0U) ,
	m_class(0U) ,
	m_rdata_offset(0U) ,
	m_rdata_size(0U)
{
	m_name = DnsMessageNameParser::read( msg , offset ) ; // NAME
	offset += DnsMessageNameParser::size( msg , offset ) ;

	m_type = msg.word( offset ) ; offset += 2U ; // TYPE
	m_class = msg.word( offset ) ; offset += 2U ; // CLASS
	offset += 4U ; // TTL
	m_rdata_size = msg.word( offset ) ; offset += 2U ; // RDLENGTH

	m_rdata_offset = offset ;
	m_size = offset - m_offset + m_rdata_size ;

	if( m_class != 1U ) // "IN" (internet)
		throw DnsMessage::Error( "invalid rr class" ) ;
}

#ifndef G_LIB_SMALL
unsigned int GNet::DnsMessageRR::type() const
{
	return m_type ;
}
#endif

bool GNet::DnsMessageRR::isa( G::string_view type_name ) const noexcept
{
	return m_type == DnsMessageRecordType::value( type_name , std::nothrow ) ;
}

unsigned int GNet::DnsMessageRR::size() const
{
	return m_size ;
}

#ifndef G_LIB_SMALL
std::string GNet::DnsMessageRR::name() const
{
	return m_name ;
}
#endif

std::string GNet::DnsMessageRR::rdataDname( unsigned int rdata_offset ) const
{
	return DnsMessageNameParser::read( m_msg , m_rdata_offset + rdata_offset ) ;
}

#ifndef G_LIB_SMALL
std::string GNet::DnsMessageRR::rdataDname( unsigned int * rdata_offset_p ) const
{
	std::string dname = DnsMessageNameParser::read( m_msg , m_rdata_offset + *rdata_offset_p ) ;
	*rdata_offset_p += DnsMessageNameParser::size( m_msg , m_rdata_offset + *rdata_offset_p ) ;
	return dname ;
}
#endif

#ifndef G_LIB_SMALL
std::string GNet::DnsMessageRR::rdataSpan( unsigned int rdata_begin ) const
{
	return rdataSpan( rdata_begin , rdataSize() ) ;
}
#endif

std::string GNet::DnsMessageRR::rdataSpan( unsigned int rdata_begin , unsigned int rdata_end ) const
{
	return m_msg.span( m_rdata_offset + rdata_begin , m_rdata_offset + rdata_end ) ;
}

#ifndef G_LIB_SMALL
unsigned int GNet::DnsMessageRR::rdataOffset() const
{
	return m_rdata_offset ;
}
#endif

unsigned int GNet::DnsMessageRR::rdataSize() const
{
	return m_rdata_size ;
}

unsigned int GNet::DnsMessageRR::rdataByte( unsigned int i ) const
{
	return m_msg.byte( m_rdata_offset + i ) ;
}

unsigned int GNet::DnsMessageRR::rdataWord( unsigned int i ) const
{
	return m_msg.word( m_rdata_offset + i ) ;
}

GNet::Address GNet::DnsMessageRR::address( unsigned int port , std::nothrow_t ) const
{
	bool ok = false ;
	return addressImp( port , ok ) ;
}

GNet::Address GNet::DnsMessageRR::address( unsigned int port ) const
{
	bool ok = false ;
	auto a = addressImp( port , ok ) ;
	if( !ok )
		throw DnsMessage::Error( "not an address" ) ;
	return a ;
}

GNet::Address GNet::DnsMessageRR::addressImp( unsigned int port , bool & ok ) const
{
	std::ostringstream ss ;
	if( isa("A") && rdataSize() == 4U )
	{
		ss << rdataByte(0U) << "." << rdataByte(1U) << "." << rdataByte(2U) << "." << rdataByte(3U) << ":" << port ;
	}
	else if( isa("AAAA") && rdataSize() == 16U )
	{
		const char * sep = "" ;
		for( unsigned int i = 0 ; i < 8U ; i++ , sep = ":" )
			ss << sep << std::hex << rdataWord(i*2U) ;
		ss << "." << port ;
	}
	ok = Address::validString( ss.str() , Address::NotLocal() ) ;
	return ok ? Address::parse( ss.str() , Address::NotLocal() ) : Address::defaultAddress() ;
}

// ==

namespace GNet
{
	//| \namespace GNet::DnsMessageRecordTypeImp
	/// A private implementation namespace for GNet::DnsMessage.
	namespace DnsMessageRecordTypeImp
	{
		struct Pair /// A std::pair-like structure used in GNet::DnsMessage, needed for gcc 4.2.1
		{
			unsigned int first ;
			const char * second ;
		} ;
		constexpr std::array<Pair,23U> map = {{
			{ 1 , "A" } , // a host address
			{ 2 , "NS" } , // an authoritative name server
			{ 3 , "MD" } , // a mail destination (Obsolete - use MX)
			{ 4 , "MF" } , // a mail forwarder (Obsolete - use MX)
			{ 5 , "CNAME" } , // the canonical name for an alias
			{ 6 , "SOA" } , // marks the start of a zone of authority
			{ 7 , "MB" } , // a mailbox domain name (EXPERIMENTAL)
			{ 8 , "MG" } , // a mail group member (EXPERIMENTAL)
			{ 9 , "MR" } , // a mail rename domain name (EXPERIMENTAL)
			{ 10 , "NULL_" } , // a null RR (EXPERIMENTAL)
			{ 11 , "WKS" } , // a well known service description
			{ 12 , "PTR" } , // a domain name pointer
			{ 13 , "HINFO" } , // host information
			{ 14 , "MINFO" } , // mailbox or mail list information
			{ 15 , "MX" } , // mail exchange
			{ 16 , "TXT" } , // text strings
			{ 28 , "AAAA" } , // IPv6 -- RFC-3596
			{ 33 , "SRV" } , // service pointer -- RFC-2782
			{ 41 , "OPT" } , // extended options -- EDNS0 -- RFC-2671
			{ 43 , "DS" } , // delegation signer -- DNSSEC -- RFC-4034
			{ 46 , "RRSIG" } , // resource record signature -- DNSSEC -- RFC-4034
			{ 47 , "NSEC" } , // next secure -- DNSSEC -- RFC-4034
			{ 48 , "DNSKEY" } // dns public key -- DNSSEC -- RFC-4034
		}} ;
	}
}

unsigned int GNet::DnsMessageRecordType::value( G::string_view type_name , std::nothrow_t ) noexcept
{
	namespace imp = DnsMessageRecordTypeImp ;
	for( const auto & item : imp::map )
	{
		if( G::Str::match( type_name , item.second ) )
			return item.first ;
	}
	return 0U ;
}

unsigned int GNet::DnsMessageRecordType::value( G::string_view type_name )
{
	unsigned int v = value( type_name , std::nothrow ) ;
	if( v == 0U )
		throw DnsMessage::Error( "invalid rr type name" ) ;
	return v ;
}

#ifndef G_LIB_SMALL
std::string GNet::DnsMessageRecordType::name( unsigned int type_value )
{
	namespace imp = DnsMessageRecordTypeImp ;
	for( const auto & item : imp::map )
	{
		if( item.first == type_value )
			return item.second ;
	}
	throw DnsMessage::Error( "invalid rr type value" ) ;
}
#endif

