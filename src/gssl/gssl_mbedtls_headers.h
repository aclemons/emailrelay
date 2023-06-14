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

#if GCONFIG_MBEDTLS_DISABLE_PSA_HEADER
// 3.3.0's psa/crypto_extra.h is broken for c++17
#define PSA_CRYPTO_EXTRA_H
#endif

#include <mbedtls/ssl.h>
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
