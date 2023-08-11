#
## Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#
# Makefile.am
#
# Use "make check" to run tests.
#
# Or, for example:
#  $ make programs ; ./emailrelay_test.sh
#  $ ./emailrelay_test.sh -q -v -t testStartsAndStops
#  $ sudo -v ; make check
#  $ su -c 'make check'
#  $ make -j 40 check TEST_LOG_FLAGS="-t -u someuser"
#
# Automake uses the TESTS variable for its list of tests, and runs a
# "compiler" according to each test's filename extension -- in practice
# "sh" for ".sh" tests and "emailrelay_test.sh" for ".test" tests.
#
# A ".test" file is created for each of the test routines defined within
# "emailrelay_test.pl".
#
# If "make check" fails then examine the relevant ".log" files and
# re-run individual tests with "emailrelay_test.sh -q -v -t <test-name>".
#

helper_programs = \
	emailrelay_test_scanner \
	emailrelay_test_client \
	emailrelay_test_server \
	emailrelay_test_dnsserver \
	emailrelay_test_verifier

helper_programs_win32 = \
	emailrelay_test_scanner.exe \
	emailrelay_test_client.exe \
	emailrelay_test_server.exe \
	emailrelay_test_dnsserver.exe \
	emailrelay_test_verifier.exe

helper_sources = \
	emailrelay_test_keygen.cpp \
	emailrelay_test_scanner.cpp \
	emailrelay_test_client.cpp \
	emailrelay_test_server.cpp \
	emailrelay_test_dnsserver.cpp \
	emailrelay_test_verifier.cpp

other_scripts = \
	emailrelay_test.sh \
	emailrelay_test.pl

test_scripts = \
	emailrelay_chain_test.sh

test_names = \
	testServerShowsHelp.test \
	testServerStartsAndStops.test \
	testServerStartsAndStopsAsRoot.test \
	testServerAdminTerminate.test \
	testSubmit.test \
	testPasswd.test \
	testPasswdDotted.test \
	testSubmitPermissions.test \
	testServerIdentityRunningAsRoot.test \
	testServerIdentityRunningSuidRoot.test \
	testServerSmtpSubmit.test \
	testServerSmtpSubmitWithPipelinedQuit.test \
	testServerPermissions.test \
	testServerPop.test \
	testServerPopDisconnect.test \
	testServerFlushNoMessages.test \
	testServerFlushNoServer.test \
	testServerFlush.test \
	testServerPolling.test \
	testServerWithBadClient.test \
	testServerSizeLimit.test \
	testClientAccountSelection.test \
	testClientContinuesIfNoSecrets.test \
	testClientSavesReasonCode.test \
	testFilter.test \
	testFilterIdentity.test \
	testFilterFailure.test \
	testFilterTimeout.test \
	testFilterWithBadFileDeletion.test \
	testFilterWithGoodFileDeletion.test \
	testFilterRescan.test \
	testFilterParallelism.test \
	testScannerPass.test \
	testScannerBlock.test \
	testScannerTimeout.test \
	testScannerOverUnixDomainSockets.test \
	testNetworkVerifierPass.test \
	testNetworkVerifierFail.test \
	testProxyConnectsOnce.test \
	testProxyServerRejection.test \
	testProxyClientFilterFails.test \
	testProxyRoutingFilterFails.test \
	testDelivery.test \
	testSplitFilter.test \
	testSplitFilterRaw.test \
	testSplitFilterWithVerifier.test \
	testRouting.test \
	testRoutingWithClientAccountSelection.test \
	testRoutingWithSplitAndMxFilters.test \
	testClientFilterPass.test \
	testClientFilterBlock.test \
	testClientNetworkFilter.test \
	testClientGivenUnknownMechanisms.test \
	testClientAuthenticationFailure.test \
	testClientMessageFailure.test \
	testClientInvalidRecipients.test \
	testClientInvalidRecipientsWithForwardToSome.test \
	testClientFailsMessagesWithNoRemoteRecipients.test \
	testTlsServerNoClientCertificateNoVerifyAccepted.test \
	testTlsServerNoClientCertificateVerifyRejected.test \
	testTlsServerNoCaAccepted.test \
	testTlsServerGoodClientCertificateNoVerifyAccepted.test \
	testTlsServerGoodClientCertificateVerifyAccepted.test \
	testTlsServerBadClientCertificateNoVerifyAccepted.test \
	testTlsServerBadClientCertificateVerifyRejected.test \
	testTlsClientGoodServerCertificateVerifyAccepted.test \
	testTlsClientBadServerCertificateVerifyRejected.test \
	testTlsClientBadServerCertificateNoVerifyRejected.test \
	testTlsGoodServerCertificateVerifyAccepted.test \
	testTlsBadServerCertificateVerifyRejected.test \
	testTlsBadServerCertificateNoVerifyAccepted.test \
	testTlsBadClientCertificateNoVerifyAccepted.test \
	testTlsBadClientCertificateVerifyRejected.test \
	testTlsNoClientTlsAndServerDoesntCare.test \
	testTlsNoClientTlsAndServerRequiresIt.test

modules = \
	AdminClient.pm \
	Check.pm \
	DnsServer.pm \
	Filter.pm \
	Helper.pm \
	NetClient.pm \
	Openssl.pm \
	OpensslCast.pm \
	OpensslFileStore.pm \
	OpensslRun.pm \
	PopClient.pm \
	Scanner.pm \
	Server.pm \
	SmtpClient.pm \
	System.pm \
	TestServer.pm \
	Verifier.pm

if GCONFIG_TESTING
TESTING_EXTRA_DIST = $(test_scripts)
TESTING_TESTS = $(test_names) $(test_scripts)
TESTING_HELPERS = $(helper_programs)
else
TESTING_EXTRA_DIST = $(test_scripts) $(helper_sources)
TESTING_TESTS =
TESTING_HELPERS =
endif

EXTRA_DIST = \
	$(TESTING_EXTRA_DIST) \
	$(other_scripts) \
	$(modules)

SUBDIRS = certificates

check_PROGRAMS = $(TESTING_HELPERS)
check_SCRIPTS = $(TESTING_TESTS)
CLEANFILES = $(test_names) .tmp.port

TESTS = $(TESTING_TESTS)

# tell automake how to run tests...
TEST_EXTENSIONS = .sh .test
TEST_LOG_COMPILER = $(top_srcdir)/test/emailrelay_test.sh
AM_TEST_LOG_FLAGS = -q -v -c $(top_srcdir)/test/certificates
SH_LOG_COMPILER = $(top_srcdir)/test/emailrelay_test.sh
AM_SH_LOG_FLAGS = --sh

AM_CPPFLAGS = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/glib \
	-I$(top_srcdir)/src/gssl \
	-I$(top_srcdir)/src/gnet \
	-D "G_SPOOLDIR=$(e_spooldir)"

COMMON_LDADD = \
	$(top_builddir)/src/glib/libglib.a

if GCONFIG_WINDOWS
 SYS_LIBS=-lws2_32 -liphlpapi -lcomctl32
else
 SYS_LIBS=
endif

emailrelay_test_scanner_SOURCES = emailrelay_test_scanner.cpp
if GCONFIG_WINDOWS
emailrelay_test_scanner_LDFLAGS = -static
endif
emailrelay_test_scanner_LDADD = \
	$(top_builddir)/src/gnet/libgnet.a \
	$(top_builddir)/src/gssl/libgssl.a \
	$(top_builddir)/src/win32/libwin32.a \
	$(COMMON_LDADD) \
	$(GCONFIG_TLS_LIBS) \
	$(SYS_LIBS)

emailrelay_test_server_SOURCES = emailrelay_test_server.cpp
if GCONFIG_WINDOWS
emailrelay_test_server_LDFLAGS = -static
endif
emailrelay_test_server_LDADD = \
	$(top_builddir)/src/gnet/libgnet.a \
	$(top_builddir)/src/gssl/libgssl.a \
	$(top_builddir)/src/win32/libwin32.a \
	$(COMMON_LDADD) \
	$(GCONFIG_TLS_LIBS) \
	$(SYS_LIBS)

emailrelay_test_dnsserver_SOURCES = emailrelay_test_dnsserver.cpp
if GCONFIG_WINDOWS
emailrelay_test_dnsserver_LDFLAGS = -static
endif
emailrelay_test_dnsserver_LDADD = \
	$(top_builddir)/src/gnet/libgnet.a \
	$(top_builddir)/src/gssl/libgssl.a \
	$(top_builddir)/src/win32/libwin32.a \
	$(COMMON_LDADD) \
	$(GCONFIG_TLS_LIBS) \
	$(SYS_LIBS)

emailrelay_test_client_SOURCES = emailrelay_test_client.cpp
if GCONFIG_WINDOWS
emailrelay_test_client_LDFLAGS = -static
endif
emailrelay_test_client_LDADD = \
	$(SYS_LIBS)

emailrelay_test_verifier_SOURCES = emailrelay_test_verifier.cpp
if GCONFIG_WINDOWS
emailrelay_test_verifier_LDFLAGS = -static
endif
emailrelay_test_verifier_LDADD = \
	$(top_builddir)/src/gnet/libgnet.a \
	$(top_builddir)/src/gssl/libgssl.a \
	$(top_builddir)/src/win32/libwin32.a \
	$(COMMON_LDADD) \
	$(GCONFIG_TLS_LIBS) \
	$(SYS_LIBS)

.PHONY: programs
if GCONFIG_WINDOWS
programs: $(helper_programs_win32) emailrelay_test_keygen.exe
else
programs: $(helper_programs)
endif

# the .test files are empty files passed to emailrelay_test.sh
$(test_names):
	-@chmod +x $(top_srcdir)/test/emailrelay_test.sh || true
	@touch $(test_names)

