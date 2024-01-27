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
/// \file gssl_mbedtls_headers.h
///

#ifndef G_SSL_MBEDTLS_HEADERS_H
#define G_SSL_MBEDTLS_HEADERS_H

#include "gdef.h"

// workround for msvc 'c linkage function cannot return
// c++ class' with mbedtls 3.x and msvc 2019 -- see also
// https://github.com/Mbed-TLS/mbedtls/issues/7087
#if !defined(GCONFIG_MBEDTLS_INCLUDE_PSA_CRYPTO_STRUCT)
	#if defined(_MSC_VER) && _MSC_VER <= 1929 && defined(__has_include)
		#if __has_include(<psa/crypto_struct.h>)
			#define GCONFIG_MBEDTLS_INCLUDE_PSA_CRYPTO_STRUCT 1
			#define GCONFIG_MBEDTLS_DISABLE_PSA_HEADER 1
		#else
			#define GCONFIG_MBEDTLS_INCLUDE_PSA_CRYPTO_STRUCT 0
		#endif
	#else
		#define GCONFIG_MBEDTLS_INCLUDE_PSA_CRYPTO_STRUCT 0
	#endif
#endif

// 3.3.0's psa/crypto_extra.h is broken for c++17
#if !defined(GCONFIG_MBEDTLS_DISABLE_PSA_HEADER)
	#define GCONFIG_MBEDTLS_DISABLE_PSA_HEADER 0
#endif

#if GCONFIG_MBEDTLS_DISABLE_PSA_HEADER
#define PSA_CRYPTO_EXTRA_H
#endif

#if GCONFIG_MBEDTLS_INCLUDE_PSA_CRYPTO_STRUCT
#include <psa/crypto_struct.h>
#endif

#include <mbedtls/ssl_ciphersuites.h>
#include <mbedtls/entropy.h>
#if GCONFIG_HAVE_MBEDTLS_NET_H
#include <mbedtls/net.h>
#else
#include <mbedtls/net_sockets.h>
#endif
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/error.h>
#include <mbedtls/version.h>
#include <mbedtls/pem.h>
#include <mbedtls/base64.h>
#include <mbedtls/debug.h>
#include <mbedtls/md5.h>
#include <mbedtls/sha1.h>
#include <mbedtls/sha256.h>
#include <mbedtls/sha512.h>

#endif
