#
## Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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

helper_programs = \
	emailrelay-test-scanner \
	emailrelay-test-client \
	emailrelay-test-server \
	emailrelay-test-verifier

helper_sources = \
	emailrelay_test_scanner.cpp \
	emailrelay_test_client.cpp \
	emailrelay_test_server.cpp \
	emailrelay_test_verifier.cpp

other_scripts = \
	emailrelay-test.pl \
	emailrelay-soak-test.sh \
	emailrelay-loop-test.sh

scripts_in = \
	emailrelay-test.sh \
	emailrelay-chain-test.sh

scripts_run = \
	emailrelay-test \
	emailrelay-chain-test

modules = \
	AdminClient.pm \
	Check.pm \
	Filter.pm \
	Helper.pm \
	Openssl.pm \
	PopClient.pm \
	Scanner.pm \
	Server.pm \
	SmtpClient.pm \
	System.pm \
	TestServer.pm \
	Verifier.pm

if GCONFIG_TESTING
TESTING_FILES_IN = $(scripts_in)
TESTING_EXTRA_DIST =
TESTING_FILES = $(scripts_run)
TESTING_HELPERS = $(helper_programs)
else
TESTING_FILES_IN =
TESTING_EXTRA_DIST = $(helper_sources)
TESTING_FILES =
TESTING_HELPERS =
endif

EXTRA_DIST = \
	$(TESTING_EXTRA_DIST) \
	$(scripts_in) \
	$(other_scripts) \
	$(modules)

SUBDIRS = certificates

check_PROGRAMS = $(TESTING_HELPERS)
check_SCRIPTS = $(TESTING_FILES)
TESTS = $(TESTING_FILES)
CLEANFILES = $(check_SCRIPTS) .emailrelay-test-server.pid

emailrelay-test: emailrelay-test.sh
	cp $(srcdir)/emailrelay-test.sh emailrelay-test
	chmod +x emailrelay-test

emailrelay-chain-test: emailrelay-chain-test.sh
	cp $(srcdir)/emailrelay-chain-test.sh emailrelay-chain-test
	chmod +x emailrelay-chain-test

AM_CPPFLAGS = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/glib \
	-I$(top_srcdir)/src/gssl \
	-I$(top_srcdir)/src/gnet \
	-D "G_SPOOLDIR=$(e_spooldir)"

COMMON_LDADD = \
	$(top_builddir)/src/glib/libglib.a

if GCONFIG_WINDOWS
 SYS_LIBS=-lws2_32 -lcomctl32
else
 SYS_LIBS=
endif

emailrelay_test_scanner_SOURCES = emailrelay_test_scanner.cpp
emailrelay_test_scanner_LDADD = \
	$(top_builddir)/src/gnet/libgnet.a \
	$(top_builddir)/src/gssl/libgssl.a \
	$(top_builddir)/src/win32/libextra.a \
	$(COMMON_LDADD) \
	$(GCONFIG_TLS_LIBS) \
	$(SYS_LIBS)

emailrelay_test_server_SOURCES = emailrelay_test_server.cpp
emailrelay_test_server_LDADD = \
	$(top_builddir)/src/gnet/libgnet.a \
	$(top_builddir)/src/gssl/libgssl.a \
	$(top_builddir)/src/win32/libextra.a \
	$(COMMON_LDADD) \
	$(GCONFIG_TLS_LIBS) \
	$(SYS_LIBS)

emailrelay_test_client_SOURCES = emailrelay_test_client.cpp
emailrelay_test_client_LDADD = \
	$(SYS_LIBS)

emailrelay_test_verifier_SOURCES = emailrelay_test_verifier.cpp
emailrelay_test_verifier_LDADD = \
	$(top_builddir)/src/gnet/libgnet.a \
	$(top_builddir)/src/gssl/libgssl.a \
	$(top_builddir)/src/win32/libextra.a \
	$(COMMON_LDADD) \
	$(GCONFIG_TLS_LIBS) \
	$(SYS_LIBS)

