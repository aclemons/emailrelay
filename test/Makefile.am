
EXTRA_DIST = \
	emailrelay-test.pl_ \
	emailrelay-test.sh_ \
	emailrelay-chain-test.sh_ \
	emailrelay-soak-test.sh_ \
	AdminClient.pm \
	Check.pm \
	Filter.pm \
	PopClient.pm \
	Port.pm \
	Scanner.pm \
	Server.pm \
	SmtpClient.pm \
	System.pm \
	TestServer.pm

noinst_SCRIPTS = emailrelay-test.sh emailrelay-test.pl emailrelay-chain-test.sh emailrelay-soak-test.sh
noinst_PROGRAMS = emailrelay-test-scanner emailrelay-test-server
TESTS = emailrelay-test.sh emailrelay-chain-test.sh
CLEANFILES = $(noinst_SCRIPTS) $(noinst_PROGRAMS)
INCLUDES = -I$(top_srcdir)/lib/$(COMPILER_VERSION) -I$(top_srcdir)/src/glib -I$(top_srcdir)/src/gnet -DG_SPOOLDIR=\"$(e_spooldir)\"

emailrelay_test_scanner_SOURCES = emailrelay_test_scanner.cpp
emailrelay_test_scanner_LDADD = \
	$(top_builddir)/src/gnet/libgnet.a \
	$(top_builddir)/src/gnet/lib$(IP).a \
	$(top_builddir)/src/glib/libglib.a \
	$(SSL_LIBS)

emailrelay_test_server_SOURCES = emailrelay_test_server.cpp
emailrelay_test_server_LDADD = \
	$(top_builddir)/src/gnet/libgnet.a \
	$(top_builddir)/src/gnet/lib$(IP).a \
	$(top_builddir)/src/glib/libglib.a \
	$(SSL_LIBS)

SUFFIXES = .pm .pl .pl_ .sh .sh_

.pl_.pl:
	cat $(top_srcdir)/test/$*.pl_ > $*.pl
	chmod +x $*.pl

.sh_.sh:
	cat $(top_srcdir)/test/$*.sh_ > $*.sh
	chmod +x $*.sh

