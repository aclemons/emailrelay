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
/// \file gssl_mbedtls_keygen.cpp
///
// See:
// * https://tls.mbed.org/kb/how-to/generate-a-self-signed-certificate
// * mbedtls/programs/pkey/gen_key.c
// * mbedtls/programs/x509/cert_write.c
//

#include "gdef.h"
#include "gssl_mbedtls_headers.h"
#include "gssl_mbedtls_keygen.h"
#include "gssl_mbedtls_utils.h"
#include <vector>
#ifdef G_WINDOWS
#include <sys/stat.h>
#include <fcntl.h>
#include <io.h>
#else
#include <fcntl.h>
#endif

namespace GSsl
{
	namespace MbedTlsImp
	{
		int open_( const char * path ) ;
		void close_( int fd ) ;
		ssize_t read_( int fd , char * p , std::size_t n ) ;
		void sleep_( int s ) ;
		void randomFillImp( char * p , std::size_t n ) ;
		int randomFill( void * , unsigned char * output , std::size_t len , std::size_t * olen ) ;
		struct FileCloser
		{
			explicit FileCloser( int fd ) : m_fd(fd) {}
			void release() { m_fd = -1 ; }
			~FileCloser() { if(m_fd >= 0) close_(m_fd) ; }
			int m_fd ;
		} ;
	}
}

GSsl::MbedTls::Error::Error( const std::string & fname , int rc ) :
	std::runtime_error( std::string("tls error: ").append(fname).append(": ").append(std::to_string(rc)) )
{
}

std::string GSsl::MbedTls::generateKey( const std::string & issuer_name )
{
	X<mbedtls_entropy_context> entropy( mbedtls_entropy_init , mbedtls_entropy_free ) ;
	if( !G::is_windows() )
	{
		const int threshold = 32 ;
		call( FN(mbedtls_entropy_add_source) , entropy.ptr() , MbedTlsImp::randomFill , nullptr ,
			threshold , MBEDTLS_ENTROPY_SOURCE_STRONG ) ;
	}

	X<mbedtls_ctr_drbg_context> drbg( mbedtls_ctr_drbg_init , mbedtls_ctr_drbg_free ) ;
	{
		std::string seed_name = "gssl_mbedtls" ;
		auto seed_name_p = reinterpret_cast<const unsigned char*>( seed_name.data() ) ;
		call( FN(mbedtls_ctr_drbg_seed) , drbg.ptr() , mbedtls_entropy_func , entropy.ptr() , seed_name_p , seed_name.size() ) ;
	}

	X<mbedtls_pk_context> key( mbedtls_pk_init , mbedtls_pk_free ) ;
	{
		const mbedtls_pk_type_t type = MBEDTLS_PK_RSA;
		const unsigned int keysize = 4096U ;
		const int exponent = 65537 ;
		call( FN(mbedtls_pk_setup) , key.ptr() , mbedtls_pk_info_from_type(type) ) ;
		call( FN(mbedtls_rsa_gen_key) , mbedtls_pk_rsa(key.x) , mbedtls_ctr_drbg_random , drbg.ptr() , keysize , exponent ) ;
	}

	std::string s_key ;
	{
		std::vector<unsigned char> pk_buffer( 16000U ) ;
		call( FN(mbedtls_pk_write_key_pem) , key.ptr() , &pk_buffer[0] , pk_buffer.size() ) ;
		pk_buffer[pk_buffer.size()-1] = 0 ;
		s_key = reinterpret_cast<const char*>( &pk_buffer[0] ) ;
	}

	// see also mbedtls/programs/x509/cert_write.c ...

	X<mbedtls_mpi> mpi( mbedtls_mpi_init , mbedtls_mpi_free ) ;
	{
		const char * serial = "1" ;
		call( FN(mbedtls_mpi_read_string) , mpi.ptr() , 10 , serial ) ;
	}

	X<mbedtls_x509write_cert> crt( mbedtls_x509write_crt_init , mbedtls_x509write_crt_free ) ;
	{
		const char * not_before = "20200101000000" ;
		const char * not_after = "20401231235959" ;
		const int is_ca = 0 ;
		const int max_pathlen = -1 ;
		call( FN(mbedtls_x509write_crt_set_subject_key) , crt.ptr() , key.ptr() ) ;
		call( FN(mbedtls_x509write_crt_set_issuer_key) , crt.ptr() , key.ptr() ) ;
		call( FN(mbedtls_x509write_crt_set_subject_name) , crt.ptr() , issuer_name.c_str()/*sic*/ ) ;
		call( FN(mbedtls_x509write_crt_set_issuer_name) , crt.ptr() , issuer_name.c_str() ) ;
		call( FN(mbedtls_x509write_crt_set_version) , crt.ptr() , MBEDTLS_X509_CRT_VERSION_3 ) ;
		call( FN(mbedtls_x509write_crt_set_md_alg) , crt.ptr() , MBEDTLS_MD_SHA256 ) ;
		call( FN(mbedtls_x509write_crt_set_serial) , crt.ptr() , mpi.ptr() ) ;
		call( FN(mbedtls_x509write_crt_set_validity) , crt.ptr() , not_before , not_after ) ;
		call( FN(mbedtls_x509write_crt_set_basic_constraints) , crt.ptr() , is_ca , max_pathlen ) ;
		call( FN(mbedtls_x509write_crt_set_subject_key_identifier) , crt.ptr() ) ;
		call( FN(mbedtls_x509write_crt_set_authority_key_identifier) , crt.ptr() ) ;
	}

	std::string s_crt ;
	{
		std::vector<unsigned char> crt_buffer( 4096 ) ;
		call( FN(mbedtls_x509write_crt_pem) , crt.ptr() , &crt_buffer[0] , crt_buffer.size() , mbedtls_ctr_drbg_random , drbg.ptr() ) ;
		crt_buffer[crt_buffer.size()-1] = 0 ;
		s_crt = reinterpret_cast<const char*>( &crt_buffer[0] ) ;
	}

	return s_key.append( s_crt ) ;
}

void GSsl::MbedTlsImp::randomFillImp( char * p , std::size_t n )
{
	// see also mbedtls/programs/pkey/gen_key.c ...

	int fd = open_( "/dev/random" ) ; // not "/dev/urandom" here
	if( fd < 0 )
		throw std::runtime_error( "cannot open /dev/random" ) ;
	FileCloser closer( fd ) ;

	while( n )
	{
		ssize_t nread_s = read_( fd , p , n ) ;
		if( nread_s < 0 )
			throw std::runtime_error( "cannot read /dev/random" ) ;

		std::size_t nread = static_cast<std::size_t>( nread_s ) ;
		if( nread > n ) // sanity check
			throw std::runtime_error( "cannot read /dev/random" ) ;

		n -= nread ;
		p += nread ;

		if( n )
			sleep_( 1 ) ; // wait for more entropy -- moot
	}
}

int GSsl::MbedTlsImp::randomFill( void * , unsigned char * output , std::size_t len , std::size_t * olen )
{
	*olen = 0U ;
	randomFillImp( reinterpret_cast<char*>(output) , len ) ;
	*olen = len ;
	return 0 ;
}

#ifdef G_WINDOWS
#pragma warning( suppress : 4996 )
int GSsl::MbedTlsImp::open_( const char * path ) { return _open( path , _O_RDONLY ) ; }
void GSsl::MbedTlsImp::close_( int fd ) { _close( fd ) ; }
ssize_t GSsl::MbedTlsImp::read_( int fd , char * p , std::size_t n ) { return _read( fd , p , static_cast<unsigned>(n) ) ; }
void GSsl::MbedTlsImp::sleep_( int s ) { Sleep( s * 1000 ) ; }
#else
int GSsl::MbedTlsImp::open_( const char * path ) { return ::open( path , O_RDONLY ) ; }
void GSsl::MbedTlsImp::close_( int fd ) { ::close( fd ) ; }
ssize_t GSsl::MbedTlsImp::read_( int fd , char * p , std::size_t n ) { return ::read( fd , p , n ) ; }
void GSsl::MbedTlsImp::sleep_( int s ) { ::sleep( s ) ; }
#endif

