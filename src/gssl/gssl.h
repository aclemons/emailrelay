//
// Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gssl.h
///
/// An interface to an underlying TLS library.
///

#ifndef G_SSL__H
#define G_SSL__H

#include "gdef.h"
#include "gstrings.h"
#include "greadwrite.h"
#include <string>
#include <utility>

namespace GSsl
{
	class Library ;
	class Profile ;
	class Protocol ;
	class Digester ;
	class LibraryImpBase ;
	class ProtocolImpBase ;
	class DigesterImpBase ;
}

/// \class GSsl::Protocol
/// A TLS protocol class. A protocol object should be constructed for each
/// secure socket. The Protocol::connect() and Protocol::accept() methods
/// are used to link the connection's i/o methods with the Protocol object.
/// Event handling for the connection is performed by the client code
/// according to the result codes from read(), write(), connect() and
/// accept().
///
/// Client code will generally need separate states to reflect an incomplete
/// read(), write(), connect() or accept() in order that they can be retried.
/// The distinction between a return code of Result_read or Result_write
/// should dictate whether the connection is put into the event loop's read
/// list or write list but it should not influence the resulting state; in
/// each state socket read events and write events can be handled identically,
/// by retrying the incomplete function call.
///
/// The protocol is half-duplex in the sense that it is not possible to read()
/// data while a write() is incomplete or write() data while a read() is
/// incomplete. (Nor is it allowed to issue a second call while the first is
/// still incomplete.)
///
class GSsl::Protocol
{
public:
	enum Result // Result enumeration for GSsl::Protocol i/o methods.
	{
		Result_ok ,
		Result_read ,
		Result_write ,
		Result_error ,
		Result_more
	} ;

	explicit Protocol( const Profile & , const std::string & peer_certificate_name = std::string() ,
		const std::string & peer_host_name = std::string() ) ;
			///< Constructor.
			///<
			///< The optional "peer-certificate-name" parameter is used as an
			///< additional check on the peer certificate. In the simplest case
			///< a client passes the server's domain name and this is checked for
			///< an exact match against the certificate's subject CNAME (eg.
			///< "CN=*.example.com"). A valid CA database is required. If the
			///< peer-certificate-name parameter is empty then a default value
			///< is taken from the profile (see Library::addProfile()).
			///<
			///< The optional "peer-host-name" parameter is included in the
			///< TLS handshake to indicate the required peer hostname. This
			///< is typcially used by clients for server-name-identification
			///< (SNI) when connecting to virtual hosts, allowing servers to
			///< assume the appropriate identity. If the peer-host-name
			///< parameter is empty then a default value is taken from the
			///< profile (see Library::addProfile()).
			///<
			///< Some underlying libraries treat peer-certificate-name and
			///< peer-host-name to be the same, using wildcard matching of
			///< the certificate CNAME against the peer-host-name.

	~Protocol() ;
		///< Destructor.

	Result connect( G::ReadWrite & io ) ;
		///< Starts the protocol actively (as a client).

	Result accept( G::ReadWrite & io ) ;
		///< Starts the protocol passively (as a server).

	Result stop() ;
		///< Initiates the protocol shutdown.

	Result read( char * buffer , size_t buffer_size_in , ssize_t & data_size_out ) ;
		///< Reads user data into the supplied buffer.
		///<
		///< Returns Result_read if there is not enough transport data to
		///< complete the internal TLS data packet. In this case the file
		///< descriptor should remain in the select() read list and the
		///< Protocol::read() should be retried using the same parameters
		///< once the file descriptor is ready to be read.
		///<
		///< Returns Result_write if the TLS layer tried to write to the
		///< file descriptor and had flow control asserted. In this case
		///< the file descriptor should be added to the select() write
		///< list and the Protocol::read() should be retried using the
		///< same parameters once the file descriptor is ready to be
		///< written.
		///<
		///< Returns Result_ok if the internal TLS data packet is complete
		///< and it has been completely deposited in the supplied buffer.
		///<
		///< Returns Result_more if the internal TLS data packet is complete
		///< and the supplied buffer was too small to take it all. In this
		///< case there will be no read event to trigger more read()s so
		///< call read() again imediately.
		///<
		///< Returns Result_error if the transport connnection was lost
		///< or if the TLS session was shut down by the peer or if there
		///< was an error.

	Result write( const char * buffer , size_t data_size_in , ssize_t & data_size_out ) ;
		///< Writes user data.
		///<
		///< Returns Result_ok if fully sent.
		///<
		///< Returns Result_read if the TLS layer needs more transport
		///< data (eg. for a renegotiation). The write() should be repeated
		///< using the same parameters on the file descriptor's next
		///< readable event.
		///<
		///< Returns Result_write if the TLS layer was blocked in
		///< writing transport data. The write() should be repeated
		///< using the same parameters on the file descriptor's next
		///< writable event.
		///<
		///< Never returns Result_more.
		///<
		///< Returns Result_error if the transport connnection was lost
		///< or if the TLS session was shut down by the peer or on error.

	static std::string str( Result result ) ;
		///< Converts a result enumeration into a printable string.
		///< Used in logging and diagnostics.

	std::string peerCertificate() const ;
		///< Returns the peer certificate in PEM format. This can be
		///< interpreted using "openssl x509 -in ... -noout -text".

	bool verified() const ;
		///< Returns true if the peer certificate has been verified.

	std::string peerCertificateChain() const ;
		///< Returns the peer certificate chain in PEM format, starting
		///< with the peer certificate and progressing towards the
		///< root CA.
		///<
		///< This is not supported by all underlying TLS libraries; the
		///< returned string may be just the peerCertificate().

private:
	Protocol( const Protocol & ) ; // not implemented
	void operator=( const Protocol & ) ; // not implemented

private:
	ProtocolImpBase * m_imp ;
} ;

/// \class GSsl::Digester
/// Returns an object that can perform a cryptographic hash.
/// Use add() one or more times, then call either state() or
/// value() and discard. The state() string can be passed
/// to the Library factory method to restart the digest
/// from the intermediate state.
///
class GSsl::Digester
{
public:
	explicit Digester( DigesterImpBase * ) ;
		///< Constructor, used by the Library class.

	size_t blocksize() const ;
		///< Returns the hash function's block size in bytes.

	size_t valuesize() const ;
		///< Returns the hash function's value size in bytes.

	size_t statesize() const ;
		///< Returns the size of the state() string in bytes,
		///< or zero if state() is not implemented.

	void add( const std::string & ) ;
		///< Adds data of arbitrary size.

	std::string state() ;
		///< Returns the intermediate state. The state string can be
		///< persisted and reused across different implementations, so
		///< it is standardised as some number of 32-bit little-endian
		///< values making up valuesize() bytes, followed by one
		///< 32-bit little-endian value holding the total add()ed
		///< size.

	std::string value() ;
		///< Returns the hash value.

private:
	shared_ptr<DigesterImpBase> m_imp ;
} ;

/// \class GSsl::Library
/// A singleton class for initialising the underlying TLS library.
/// The library is configured with one or more named "profiles", and
/// Protocol objects are constructed with reference to a particular
/// profile. Typical profile names are "server" and "client".
///
class GSsl::Library
{
public:
	typedef void (*LogFn)( int , const std::string & ) ;

	explicit Library( bool active = true , const std::string & library_config = std::string() ,
		LogFn = Library::log , bool verbose = true ) ;
			///< Constructor.
			///<
			///< The 'active' parameter can be set to false as an optimisation if the
			///< library is not going to be used; calls to addProfile() will do
			///< nothing, calls to hasProfile() will return false, and calls to
			///< profile() will throw.
			///<
			///< The library-config parameter should be empty by default; the format
			///< and interpretation are undefined at this interface.

	~Library() ;
		///< Destructor. Cleans up the underlying TLS library.

	static void log( int level , const std::string & line ) ;
		///< The default logging callback function, where the level is 1 for
		///< debug, 2 for info, 3 for warnings, and 4 for errors. There
		///< will be no level 1 logging if the constructor's 'verbose'
		///< flag was false.

	static Library * instance() ;
		///< Returns a pointer to a library object, if any.

	void addProfile( const std::string & profile_name , bool is_server_profile ,
		const std::string & key_file = std::string() , const std::string & cert_file = std::string() ,
		const std::string & ca_path = std::string() ,
		const std::string & default_peer_certificate_name = std::string() ,
		const std::string & default_peer_host_name = std::string() ,
		const std::string & profile_config = std::string() ) ;
			///< Creates a named Profile object that can be retrieved by profile().
			///<
			///< A typical application will have two profiles named "client" and "server".
			///< The "is-server-profile" flag indicates whether Protocol::connect()
			///< or Protocol::accept() will be used.
			///<
			///< The "key-file" and "cert-file" parameters point to a PEM files containing
			///< our own key and certificate, and this can be the same file if it contains
			///< both. These are required if acting as a server, but if not supplied
			///< this method will succeed with the failures occuring in any subsequent
			///< server-side session setup.
			///<
			///< The "ca-path" parameter points to a file or directory containing a
			///< database of CA certificates used for peer certificate verification.
			///< If this is "<none>" then a server will not ask its client for a
			///< certificate; if it is empty then the peer certificate will be requested,
			///< but the server will not require a certificate from the client, and
			///< any certificate received will not be not verified; if it is a file
			///< system path or "<default>" then a peer certificate will be required
			///< and it will be verified against the CA database.
			///<
			///< The "default-peer-certificate-name" parameter is used by Protocol
			///< objects created from this Profile in cases when the Protocol does not
			///< get a more specific peer-certificate-name passed in its constructor.
			///<
			///< Similarly the "default-peer-host-name" is used by Protocol objects
			///< if they do not get a more specific peer-host-name in their constructor.
			///<
			///< The "profile-config" parameter is used for any additional configuration
			///< items; the format and interpretation are undefined at this interface.

	bool hasProfile( const std::string & profile_name ) const ;
		///< Returns true if the named profile has been add()ed.

	const Profile & profile( const std::string & profile_name ) const ;
		///< Returns an opaque reference to the named profile. The profile
		///< can be used to construct a protocol instance.

	bool enabled() const ;
		///< Returns true if this is a real TLS library and the constructor's active
		///< parameter was set.

	std::string id() const ;
		///< Returns the TLS library name and version.

	static LibraryImpBase & impstance() ;
		///< Returns a reference to the pimple object when enabled(). Used in
		///< implementations. Throws if none.

	static bool real() ;
		///< Returns true if this is a real TLS library.

	static std::string credit( const std::string & prefix , const std::string & eol , const std::string & eot ) ;
		///< Returns a multi-line library credit for all available TLS libraries.

	static std::string ids() ;
		///< Returns a concatenation of all available TLS library names and versions.

	static bool enabledAs( const std::string & profile_name ) ;
		///< A static convenience function that returns true if there is an
		///< enabled() Library instance() that has the named profile.

	static G::StringArray digesters( bool require_state = false ) ;
		///< Returns a list of hash function names (such as "MD5") that the TLS
		///< library can do, ordered roughly from strongest to weakest. Returns
		///< the empty list if there is no Library instance. If the boolean
		///< parameter is true then the returned list is limited to those
		///< hash functions that can be initialised with an intermediate
		///< state.

	Digester digester( const std::string & name , const std::string & state = std::string() ) const ;
		///< Returns a digester object.

private:
	Library( const Library & ) ;
	void operator=( const Library & ) ;
	const LibraryImpBase & imp() const ;
	LibraryImpBase & imp() ;
	static LibraryImpBase * newLibraryImp( G::StringArray & , Library::LogFn , bool ) ;

private:
	static Library * m_this ;
	LibraryImpBase * m_imp ;
} ;

/// \class GSsl::LibraryImpBase
/// A base interface for GSsl::Library pimple classes. A common base allows
/// for multiple TLS libraries to be built in and then selected at run-time.
///
class GSsl::LibraryImpBase
{
public:
	virtual ~LibraryImpBase() ;
		///< Destructor.

	virtual std::string id() const = 0 ;
		///< Implements Library::id().

	virtual void addProfile( const std::string & , bool , const std::string & , const std::string & ,
		const std::string & , const std::string & , const std::string & , const std::string & ) = 0 ;
			///< Implements Library::addProfile().

	virtual bool hasProfile( const std::string & profile_name ) const = 0 ;
		///< Implements Library::hasProfile().

	virtual const Profile & profile( const std::string & profile_name ) const = 0 ;
		///< Implements Library::profile().

	virtual G::StringArray digesters( bool ) const = 0 ;
		///< Implements Library::digesters().

	virtual Digester digester( const std::string & , const std::string & ) const = 0 ;
		///< Implements Library::digester().

	static bool consume( G::StringArray & list , const std::string & item ) ;
		///< A convenience function that removes the item from
		///< the list and returns true iff is was removed.
} ;

/// \class GSsl::Profile
/// A base interface for profile classes that work with concrete classes
/// derived from GSsl::LibraryImpBase and GSsl::ProtocolImpBase.
///
class GSsl::Profile
{
public:
	virtual ~Profile() ;
		///< Destructor.

	virtual ProtocolImpBase * newProtocol( const std::string & , const std::string & ) const = 0 ;
		///< Factory method for a new Protocol object on the heap.
} ;

/// \class GSsl::ProtocolImpBase
/// A base interface for GSsl::Protocol pimple classes.
///
class GSsl::ProtocolImpBase
{
public:
	virtual ~ProtocolImpBase() ;
		///< Destructor.

	virtual Protocol::Result connect( G::ReadWrite & ) = 0 ;
		///< Implements Protocol::connect().

	virtual Protocol::Result accept( G::ReadWrite & ) = 0 ;
		///< Implements Protocol::accept().

	virtual Protocol::Result stop() = 0 ;
		///< Implements Protocol::stop().

	virtual Protocol::Result read( char * , size_t , ssize_t & ) = 0 ;
		///< Implements Protocol::read().

	virtual Protocol::Result write( const char * , size_t , ssize_t & ) = 0 ;
		///< Implements Protocol::write().

	virtual std::string peerCertificate() const = 0 ;
		///< Implements Protocol::peerCertificate().

	virtual std::string peerCertificateChain() const = 0 ;
		///< Implements Protocol::peerCertificateChain().

	virtual bool verified() const = 0 ;
		///< Implements Protocol::verified().
} ;

/// \class GSsl::DigesterImpBase
/// A base interface for GSsl::Digester pimple classes.
///
class GSsl::DigesterImpBase
{
public:
	virtual ~DigesterImpBase() ;
		///< Destructor.

	virtual void add( const std::string & ) = 0 ;
		///< Implements Digester::add().

	virtual std::string value() = 0 ;
		///< Implements Digester::value().

	virtual std::string state() = 0 ;
		///< Implements Digester::state().

	virtual size_t blocksize() const = 0 ;
		///< Implements Digester::blocksize().

	virtual size_t valuesize() const = 0 ;
		///< Implements Digester::valuesize().

	virtual size_t statesize() const = 0 ;
		///< Implements Digester::statesize().
} ;

#endif
