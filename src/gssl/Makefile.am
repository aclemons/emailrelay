#
## Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
## 
## This program is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
## 
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
## 
## You should have received a copy of the GNU General Public License
## along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

if GCONFIG_TLS_USE_NONE
SSL_EXTRA_DIST = gssl.cpp gssl_openssl.cpp gssl_openssl.h gssl_mbedtls.cpp gssl_mbedtls.h gssl_use_both.cpp gssl_use_openssl.cpp gssl_use_mbedtls.cpp
SSL_SOURCES = gssl.h gssl_none.cpp
endif

if GCONFIG_TLS_USE_BOTH
SSL_EXTRA_DIST = gssl_none.cpp gssl_use_openssl.cpp gssl_use_mbedtls.cpp
SSL_SOURCES = gssl.h gssl.cpp gssl_openssl.cpp gssl_openssl.h gssl_mbedtls.cpp gssl_mbedtls.h gssl_use_both.cpp
endif

if GCONFIG_TLS_USE_OPENSSL
SSL_EXTRA_DIST = gssl_none.cpp gssl_mbedtls.cpp gssl_mbedtls.h gssl_use_mbedtls.cpp gssl_use_both.cpp
SSL_SOURCES = gssl.h gssl.cpp gssl_openssl.cpp gssl_openssl.h gssl_use_openssl.cpp
endif

if GCONFIG_TLS_USE_MBEDTLS
SSL_EXTRA_DIST = gssl_none.cpp gssl_openssl.cpp gssl_openssl.h gssl_use_openssl.cpp gssl_use_both.cpp
SSL_SOURCES = gssl.h gssl.cpp gssl_mbedtls.cpp gssl_mbedtls.h gssl_use_mbedtls.cpp
endif

EXTRA_DIST = \
	$(SSL_EXTRA_DIST)

AM_CPPFLAGS = -I$(top_srcdir)/src/glib
noinst_LIBRARIES = libgssl.a

libgssl_a_SOURCES = $(SSL_SOURCES)

