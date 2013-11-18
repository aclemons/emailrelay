#
## Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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

if TESTING
TESTING_FILES = emailrelay-test.sh emailrelay-chain-test.sh
TESTING_EXTRA_DIST =
TESTING_HELPERS = emailrelay-test-scanner emailrelay-test-server emailrelay-test-verifier
else
TESTING_FILES =
TESTING_EXTRA_DIST =
TESTING_HELPERS =
endif

EXTRA_DIST = \
	$(TESTING_EXTRA_DIST) \
	emailrelay-test.pl_ \
	emailrelay-test.sh_ \
	emailrelay-chain-test.sh_ \
	emailrelay-soak-test.sh_ \
	emailrelay-loop-test.sh_ \
	AdminClient.pm \
	Check.pm \
	Filter.pm \
	Helper.pm \
	PopClient.pm \
	Port.pm \
	Scanner.pm \
	Server.pm \
	SmtpClient.pm \
	System.pm \
	TestServer.pm \
	Verifier.pm \
	mingw.mak

noinst_SCRIPTS = emailrelay-test.sh emailrelay-test.pl emailrelay-chain-test.sh emailrelay-soak-test.sh emailrelay-loop-test.sh
noinst_PROGRAMS = $(TESTING_HELPERS)
TESTS = $(TESTING_FILES)
CLEANFILES = $(noinst_SCRIPTS) $(noinst_PROGRAMS) .emailrelay-test-server.pid
INCLUDES = -I$(top_srcdir)/lib/$(COMPILER_VERSION) -I$(top_srcdir)/src/glib -I$(top_srcdir)/src/gssl -I$(top_srcdir)/src/gnet -DG_SPOOLDIR=\"$(e_spooldir)\"

COMMON_LDADD = \
	$(top_builddir)/src/glib/libglib.a

emailrelay_test_scanner_SOURCES = emailrelay_test_scanner.cpp
emailrelay_test_scanner_LDADD = \
	$(top_builddir)/src/gnet/libgnet.a \
	$(top_builddir)/src/gssl/libgssl.a \
	$(COMMON_LDADD) \
	$(SSL_LIBS)

emailrelay_test_server_SOURCES = emailrelay_test_server.cpp
emailrelay_test_server_LDADD = \
	$(top_builddir)/src/gnet/libgnet.a \
	$(top_builddir)/src/gssl/libgssl.a \
	$(COMMON_LDADD) \
	$(SSL_LIBS)

emailrelay_test_verifier_SOURCES = emailrelay_test_verifier.cpp
emailrelay_test_verifier_LDADD = \
	$(top_builddir)/src/gnet/libgnet.a \
	$(top_builddir)/src/gssl/libgssl.a \
	$(COMMON_LDADD) \
	$(SSL_LIBS)

SUFFIXES = .pm .pl .pl_ .sh .sh_

.pl_.pl:
	cat "$(top_srcdir)/test/$*.pl_" > "$*.pl"
	chmod +x "$*.pl"

.sh_.sh:
	cat "$(top_srcdir)/test/$*.sh_" > "$*.sh"
	chmod +x "$*.sh"

