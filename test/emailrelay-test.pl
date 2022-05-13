#!/usr/bin/env perl
#
# Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
# ===
#
# emailrelay-test.pl
#
# Tests the E-MailRelay system. Individual named tests are defined in
# testWhatever() subroutines. By default all tests are run (or skipped)
# in sequence.
#
# usage:
#   emailrelay-test.pl { list | fixup ... }
#   emailrelay-test.pl [-d <bin-dir>] [-x <testbin-dir>] [-o <openssltool-dir>] [-c <certs-dir>] [-k] [-v] [-t] [-T <config>] [<test-name> ...]
#      -d  - directory containing emailrelay binary
#      -x  - directory containing test program binaries
#      -o  - directory containing the openssl tool
#      -c  - directory containing test certificates
#      -k  - keep going after a failed test
#      -v  - verbose logging from this script
#      -q  - no 'skipping' messages
#      -t  - keep temporary files
#      -u  - unprivileged login account name (if running as root)
#      -T  - tls-config (default "-tlsv1.2") (eg. "-T mbedtls")
#      -O  - options for openssl s_client and s_server
#      -C  - create certs and exit
#      -V  - use valgrind
#
# Giving one or more test names on the command-line has the effect of
# skipping the others, or "-<test-name>" can be used to skip a specific
# test. Test names can optionally be decorated with a ".test" suffix,
# for the convenience of automake.
#
# Tests might be skipped if not run as root, or for other reasons. If all
# tests are skipped then a special exit code of 77 is used.
#
# Timing parameters might need tweaking depending on the speed of the
# machine.
#

use strict ;
die "emailrelay-test.pl: dont use msys perl" if $^O eq "msys" ;
use Carp ;
use FileHandle ;
use Getopt::Std ;
use File::Basename ;
use lib File::Basename::dirname($0) ;
use Server ;
use TestServer ;
use Helper ;
use AdminClient ;
use SmtpClient ;
use PopClient ;
use Check ;
use System ;
use Scanner ;
use Verifier ;
use Openssl ;
use OpensslCast ;
use OpensslRun ;
use OpensslFileStore ;
use Filter ;

$| = 1 ;

# parse the command line
my %opts = () ;
getopts( 'd:o:x:c:CkvqtT:VO:u:' , \%opts ) or die ;
my $opt_bin_dir = $opts{d} || "../src/main" ;
my $opt_test_bin_dir = $opts{x} || "." ;
my $opt_certs_dir = $opts{c} || "certificates" ;
my $opt_keep_going = exists $opts{k} ;
my $opt_quiet = exists $opts{q} ;
my $opt_test_account = $opts{u} ;
$Openssl::openssl = Openssl::search( $opts{o} , System::windows() ) ;
$OpensslRun::openssl = $Openssl::openssl ;
$OpensslRun::client_options = exists $opts{O} ? $opts{O} : undef ;
$OpensslRun::server_options = exists $opts{O} ? $opts{O} : undef ;
$OpensslRun::log_run_fn = sub { System::log_("running [$_[0]]($_[1])") } ;
$System::bin_dir = $opt_bin_dir ;
$System::localhost = "127.0.0.1" ; # in case localhost resolves to ipv6 first
$System::verbose = 1 if exists $opts{v} ;
$System::keep = 1 if exists $opts{t} ;
$Server::tls_config = exists $opts{T} ? $opts{T} : "-tlsv1.2" ;
$Server::with_valgrind = $opts{V} if exists $opts{V} ;
$Server::bin_dir = $opt_bin_dir ;
$Helper::bin_dir = $opt_test_bin_dir ;
$TestServer::bin_dir = $opt_test_bin_dir ;
$Openssl::keep = 1 if exists $opts{t} ;
$Openssl::log_fn = sub { System::log_("running [$_[0]] ($_[1])") } ;
$OpensslFileStore::log_cat_fn = sub { System::log_("creating [$_[0]]") } ;
$OpensslFileStore::unlink_fn = sub { System::unlink($_[0]) } ;
$OpensslFileStore::outpath_fn = sub { System::tempfile($_[1]) } ;
$OpensslFileStore::inpath_fn = sub { $opt_certs_dir?"$opt_certs_dir/$_[1]":$_[1] } ;
createCerts() if exists($opts{C}) ;

my $run_all_ok = 1 ;

sub requireUnix
{
	die "skipped: not unix\n"
		if !System::unix() ;
}

sub requireUnixDomainSockets
{
	requireUnix() ; # could do better -- assumes '--with-uds'
}

sub requireRoot
{
	# (typically called after requireUnix())
	die "skipped: not root\n"
		if !System::amRoot() ;
}

sub requireSudo
{
	die "skipped: no sudo\n"
		if !System::haveSudo() ;
}

sub requireRootOrSudo
{
	die "skipped: not root and no sudo\n"
		if ( !System::amRoot() && !System::haveSudo() ) ;
}

sub requireDebug
{
	my $server = new Server() ;
	die "skipped: not a debug build\n"
		if !$server->hasDebug() ;
}

sub requireThreads
{
	my $server = new Server() ;
	my $has_threads = $server->hasThreads() ;
	$server->cleanup() ;
	die "skipped: not multi-threaded\n"
		if !$has_threads ;
}

sub requireTestAccount
{
	my $name = $opt_test_account || System::testAccount() ;
	die "skipped: no non-root test account\n"
		if( !$name || $name eq "root" ) ;
}

sub requireOpensslTool
{
	die "skipped: no openssl tool\n"
		if !Openssl::available() ;
}

sub requireTls
{
	my $server = new Server() ;
	my $has_tls = $server->hasTls() ;
	$server->cleanup() ;
	die "skipped: no tls\n"
		if !$has_tls ;
}

sub createCerts
{
	requireOpensslTool() ;
	mkdir $opt_certs_dir if ! -d $opt_certs_dir ;
	OpensslCast::create( $opt_certs_dir ) ;
	exit( 0 ) ;
}

# ===

sub testServerShowsHelp
{
	# setup
	my %args = (
		Help => 1 ,
	) ;
	requireUnix() ; # could do better
	my $server = new Server() ;

	# test help output
	Check::ok( $server->run(\%args,undef,undef,0) , "failed to run" , $server->message() , $server->command() ) ;
	Check::that( $server->rc() == 0 , "non-zero exit" ) ;
	Check::fileNotEmpty( $server->stdout() ) ;
	Check::fileEmpty( $server->stderr() ) ;

	# tear down
	$server->cleanup() ;
}

sub testServerStartsAndStops
{
	_testServerStartsAndStops( 0 ,
		AsServer => 1 ,
		Domain => 1 ,
		Port => 1 ,
		PidFile => 1 ,
		SpoolDir => 1 ,
	) ;
}

sub testServerStartsAndStopsAsRoot
{
	requireUnix() ;
	requireRootOrSudo() ;
	_testServerStartsAndStops( 1 ,
		AsServer => 1 ,
		Domain => 1 ,
		Port => 1 ,
		PidFile => 1 ,
		SpoolDir => 1 ,
		User => 1 ,
	) ;
}

sub _testServerStartsAndStops
{
	# setup
	my ( $as_root , %args ) = @_ ;
	requireUnix() ;
	my $server = new Server() ;
	my $sudo_prefix = $as_root ? System::sudoPrefix() : "" ;

	# test that the server starts up
	Check::ok( $server->run(\%args,$sudo_prefix) , "failed to run" , $server->message() ) ;
	Check::that( $server->rc() == 0 , "immediate error" ) ;
	Check::fileExists( $server->stdout() ) ;
	Check::fileExists( $server->stderr() ) ;
	Check::fileExists( $server->pidFile() , "pid file" ) ;
	Check::numeric( $server->pid() , "pid file" ) ;
	Check::running( $server->pid() , $server->message() ) ;

	# test that the server stops on a signal
	my $smtp_client = new SmtpClient( $server->smtpPort() , $System::localhost ) ;
	Check::ok( $smtp_client->open(0) , "cannot connect for smtp" , $server->smtpPort() ) ;
	$server->kill() ;
	Check::notRunning( $server->pid() ) ;
	Check::fileDeleted( $server->pidFile() , "pid file" ) if System::unix() ;
	Check::fileEmpty( $server->stdout() ) ;
	Check::fileDoesNotContain( $server->stderr() , [ "error" , "warning" , "exception" , "core" ] , "stderr" ) ;

	# tear down
	$server->cleanup() ;
}

sub testServerAdminTerminate
{
	# setup
	my %args = (
		AsServer => 1 ,
		Domain => 1 ,
		Port => 1 ,
		Admin => 1 ,
		PidFile => 1 ,
		SpoolDir => 1 ,
		AdminTerminate => 1 ,
	) ;
	my $server = new Server() ;
	Check::ok( $server->run(\%args) , "failed to run" , $server->message() ) ;
	Check::that( $server->rc() == 0 , "immediate error" ) ;
	Check::running( $server->pid() , $server->message() ) ;

	# test that the server can be terminated through the admin interface
	my $admin_client = new AdminClient( $server->adminPort() , $System::localhost ) ;
	Check::ok( $admin_client->open() , "cannot connect for admin" , $server->adminPort() ) ;
	$admin_client->doHelp() ;
	$admin_client->doHelp() ;
	$admin_client->doTerminate() ;
	$server->wait() ;
	Check::notRunning( $server->pid() ) ;
	Check::fileDeleted( $server->pidFile() , "pid file" ) if System::unix() ;
	Check::fileDoesNotContain( $server->stderr() , [ "error" , "warning" , "exception" , "core" ] , "stderr" ) ;

	# tear down
	$server->cleanup() ;
}

sub testSubmit
{
	# setup
	my $spool_dir = System::createSpoolDir() ;
	my $path = System::createSmallMessageFile() ;

	# test that the submit utility works
	my $exe = System::sanepath( System::exe( $opt_bin_dir , "emailrelay-submit" ) ) ;
	my $cmd = System::commandline( "$exe --from me\@here.localnet --spool-dir $spool_dir me\@there.localnet < $path" , {background=>0} ) ;
	my $rc = system( $cmd ) ;
	Check::that( $rc == 0 , "failed to submit" ) ;
	Check::fileMatchCount( $spool_dir."/emailrelay.*.content" , 1 ) ;
	Check::fileMatchCount( $spool_dir."/emailrelay.*.envelope" , 1 ) ;
	Check::fileNotEmpty( System::match($spool_dir."/emailrelay.*.content") ) ;
	Check::fileNotEmpty( System::match($spool_dir."/emailrelay.*.envelope") ) ;

	# tear down
	System::deleteSpoolDir($spool_dir) ;
	System::unlink( $path ) ;
}

sub testPasswd
{
	# test that the password utility works
	my $exe = System::sanepath( System::exe( $opt_bin_dir , "emailrelay-passwd" ) ) ;
	my $fh = new FileHandle( "echo foobar | $exe |" ) ;
	my $result = <$fh> ;
	chomp $result ;
	my $base64 = "3pIN0aPbd5aN0S4ogKA3w2P3P9XH0hXchLP1W+adpQE=" ;
	my $ok = ( $result eq $base64 ) ;
	Check::that( $ok , "password digest generation failed" ) ;
}

sub testPasswdDotted
{
	# test that the password utility works in backwards-compatibility mode
	my $exe = System::sanepath( System::exe( $opt_bin_dir , "emailrelay-passwd" ) ) ;
	my $fh = new FileHandle( "echo foobar | $exe --dotted |" ) ;
	my $result = <$fh> ;
	chomp $result ;
	my $dotted = "3507327710.2524437411.674156941.3275202688.3577739107.3692417735.1542828932.27631078" ;
	my $ok = ( $result eq $dotted ) ;
	Check::that( $ok , "password digest generation failed" ) ;
}

sub testSubmitPermissions
{
	# setup -- group-suid-daemon exe and group-daemon spool directory
	requireUnix() ;
	requireRoot() ;
	requireTestAccount() ;
	my $submit_exe = System::tempfile( "submit" ) ;
	my $rc = system( "cp $opt_bin_dir/emailrelay-submit $submit_exe" ) ;
	$rc += system( "chown daemon:daemon $submit_exe" ) ;
	$rc += system( "chmod 755 $submit_exe" ) ;
	$rc += system( "chmod g+s $submit_exe" ) ;
	Check::that( $rc == 0 , "cannot create suid submit executable" , $submit_exe ) ;
	my $spool_dir = System::createSpoolDir( "spool" , "daemon" ) ;
	my $message_file = System::createSmallMessageFile() ;
	$rc = system( "chmod 440 $message_file" ) ;
	Check::that( $rc == 0 , "cannot set file permissions" , $message_file ) ;

	# test that group-suid-daemon submit executable creates files correctly when run from some random test-account
	my $cmd = "$submit_exe --from me\@here.localnet --spool-dir $spool_dir me\@there.localnet" ;
	my $someuser = $opt_test_account || System::testAccount() ;
	$rc = system( "cat $message_file | su -m $someuser -c \"$cmd\"" ) ;
	Check::that( $rc == 0 , "failed to submit" ) ;
	Check::fileMatchCount( $spool_dir."/emailrelay.*.content" , 1 ) ;
	Check::fileMatchCount( $spool_dir."/emailrelay.*.envelope" , 1 ) ;
	Check::fileNotEmpty( System::match($spool_dir."/emailrelay.*.content") ) ;
	Check::fileNotEmpty( System::match($spool_dir."/emailrelay.*.envelope") ) ;
	Check::fileOwner( System::match($spool_dir."/emailrelay.*.content") , $someuser ) ;
	Check::fileOwner( System::match($spool_dir."/emailrelay.*.envelope") , $someuser ) ;
	Check::fileGroup( System::match($spool_dir."/emailrelay.*.content") , "daemon" ) ;
	Check::fileGroup( System::match($spool_dir."/emailrelay.*.envelope") , "daemon" ) ;
	Check::fileMode( System::match($spool_dir."/emailrelay.*.content") , 0660 ) ;
	Check::fileMode( System::match($spool_dir."/emailrelay.*.envelope") , 0660 ) ;

	# tear down
	System::deleteSpoolDir($spool_dir) ;
	System::unlink( $message_file ) ;
	System::unlink( $submit_exe ) ;
}

sub testServerIdentityRunningAsRoot
{
	# setup
	my %args = (
		AsServer => 1 ,
		Domain => 1 ,
		Port => 1 ,
		PidFile => 1 ,
		SpoolDir => 1 ,
		User => 1 ,
	) ;
	requireUnix() ;
	requireRootOrSudo() ;
	my $server = new Server() ;
	$server->run( \%args , System::sudoPrefix() ) ;
	Check::running( $server->pid() , $server->message() ) ;

	# test that the effective id comes from --user
	Check::processRealUser( $server->pid() , "root" ) ;
	Check::processRealGroup( $server->pid() , "root" ) ;
	Check::processEffectiveUser( $server->pid() , $server->user() ) ;
	Check::processEffectiveGroup( $server->pid() , $server->user() ) ;

	# tear down
	$server->kill() ;
	$server->cleanup() ;
}

sub testServerIdentityRunningSuidRoot
{
	# setup
	my %args = (
		AsServer => 1 ,
		Domain => 1 ,
		Port => 1 ,
		PidFile => 1 ,
		SpoolDir => 1 ,
		User => 1 ,
	) ;
	requireUnix() ;
	requireRootOrSudo() ;
	requireTestAccount() ;
	my $server = new Server() ;
	my $exe = System::tempfile( "emailrelay" ) ;
	my $rc = system( "cp ".$server->exe()." $exe" ) ;
	$rc += system( "chmod 775 \"$exe\"" ) ;
	$rc += system( System::sudoCommand("chown root \"$exe\"") ) ;
	$rc += system( System::sudoCommand("chmod u+s \"$exe\"") ) ;
	Check::that( $rc == 0 ) ;
	$server->set_exe( $exe ) ;
	my $someuser = $opt_test_account || System::testAccount() ;
	$server->run( \%args , System::sudoUserPrefix($someuser) ) ;
	Check::running( $server->pid() , $server->message() ) ;

	# test that the server has given up root privileges
	Check::processRealUser( $server->pid() , $someuser ) ;
	Check::processEffectiveUser( $server->pid() , $someuser ) ;
	#Check::processSavedUser( $server->pid() , "root" ) ; # fails if mount point is 'nosuid'

	# tear down
	$server->kill() ;
	$server->cleanup() ;
	System::unlink( $exe ) ;
}

sub testServerSmtpSubmit
{
	_testServerSmtpSubmit() ;
}

sub testServerSmtpSubmitWithPipelinedQuit
{
	_testServerSmtpSubmit( 1 ) ;
}

sub _testServerSmtpSubmit
{
	my ( $pipelined_quit ) = @_ ;

	# setup
	my %args = (
		Log => 1 ,
		Verbose => 1 ,
		Domain => 1 ,
		Port => 1 ,
		PidFile => 1 ,
		SpoolDir => 1 ,
		Debug => 1 ,
	) ;
	my $server = new Server() ;
	$server->run( \%args ) ;
	Check::running( $server->pid() , $server->message() ) ;
	my $smtp_client = new SmtpClient( $server->smtpPort() , $System::localhost ) ;
	$smtp_client->open() ;
	$smtp_client->submit_start() ;
	my $line = "lkjldkjfglkjdfglkjdferoiwuoiruwoeiur" ;
	for( my $i = 0 ; $i < 100 ; $i++ ) { $smtp_client->submit_line($line) }
	$smtp_client->submit_end_nowait( 1 ) if $pipelined_quit ;
	$smtp_client->submit_end() unless $pipelined_quit ;

	# test that message files appear in the spool directory
	System::waitForFiles( $server->spoolDir()."/emailrelay.*.envelope" , 1 ) if $pipelined_quit ;
	Check::fileMatchCount( $server->spoolDir()."/emailrelay.*.content" , 1 ) ;
	Check::fileMatchCount( $server->spoolDir()."/emailrelay.*.envelope" , 1 ) ;
	Check::fileNotEmpty( System::match($server->spoolDir()."/emailrelay.*.envelope") ) ;
	my $content = System::match($server->spoolDir()."/emailrelay.*.content") ;
	Check::fileNotEmpty( $content ) ;
	Check::fileLineCount( $content , 1 , "Received" ) ;
	Check::fileLineCount( $content , 100 , $line ) ;

	# tear down
	$server->kill() ;
	$server->cleanup() ;
}

sub testServerPermissions
{
	# setup
	my %args = (
		Log => 1 ,
		Verbose => 1 ,
		Port => 1 ,
		PidFile => 1 ,
		SpoolDir => 1 ,
		User => 1 ,
		Domain => 1 ,
		#Debug => 1 ,
	) ;
	requireUnix() ;
	requireRootOrSudo() ;
	my $server = new Server() ;
	# (on bsd new files' group-ids always inherit from the directory,
	# but on linux it depends on the directory's set-group-id bit)
	my $bsd = ( System::bsd() || System::mac() ) ;
	my $rc = system( "chmod g-s " . $server->spoolDir() ) ;
	$server->run( \%args , System::sudoPrefix() ) ;
	Check::running( $server->pid() , $server->message() ) ;
	my $smtp_client = new SmtpClient( $server->smtpPort() , $System::localhost ) ;
	$smtp_client->open() ;
	$smtp_client->submit() ;

	# test permissions of created files
	Check::fileOwner( $server->pidFile() , "root" ) ;
	Check::fileGroup( $server->pidFile() , $server->user() ) unless $bsd ;
	Check::fileOwner( System::match($server->spoolDir()."/emailrelay.*.content") , "root" ) ;
	Check::fileGroup( System::match($server->spoolDir()."/emailrelay.*.content") , $server->user() ) unless $bsd ;
	Check::fileMode( System::match($server->spoolDir()."/emailrelay.*.content") , 0660 ) ;
	Check::fileOwner( System::match($server->spoolDir()."/emailrelay.*.envelope") , "root" ) ;
	Check::fileGroup( System::match($server->spoolDir()."/emailrelay.*.envelope") , $server->user() ) unless $bsd ;
	Check::fileMode( System::match($server->spoolDir()."/emailrelay.*.envelope") , 0660 ) ;

	# tear down
	$server->kill() ;
	$server->cleanup() ;
}

sub testServerPop
{
	# setup
	my %args = (
		Pop => 1 ,
		PopAuth => 1 ,
		PopPort => 1 ,
		NoSmtp => 1 ,
		Log => 1 ,
		Verbose => 1 ,
		Domain => 1 ,
		PidFile => 1 ,
		SpoolDir => 1 ,
	) ;
	my $server = new Server() ;
	System::createFile( $server->popSecrets() , "server login me secret" ) ;
	$server->run( \%args ) ;
	Check::running( $server->pid() , $server->message() ) ;
	my $pop_client = new PopClient( $server->popPort() , $System::localhost ) ;
	System::submitSmallMessage( $server->spoolDir() ) ;
	System::submitSmallMessage( $server->spoolDir() ) ;
	System::submitSmallMessage( $server->spoolDir() ) ;
	System::chmod_r( $server->spoolDir() , "700" , "600" ) ;

	# test that the pop client can log in and get a message list
	Check::ok( $pop_client->open() , "cannot connect to pop port" ) ;
	Check::ok( $pop_client->login( "me" , "secret" ) ) ;
	my @list = $pop_client->list() ;
	Check::that( scalar(@list) == 3 , "invalid message list size" , scalar(@list) ) ;

	# tear down
	$server->kill() ;
	$server->cleanup() ;
}

sub testServerPopDisconnect
{
	# setup
	my %args = (
		Pop => 1 ,
		PopAuth => 1 ,
		PopPort => 1 ,
		NoSmtp => 1 ,
		Log => 1 ,
		Verbose => 1 ,
		Domain => 1 ,
		PidFile => 1 ,
		SpoolDir => 1 ,
		Debug => 1 ,
	) ;
	my $server = new Server() ;
	System::createFile( $server->popSecrets() , "server login me secret" ) ;
	$server->run( \%args ) ;
	Check::running( $server->pid() , $server->message() ) ;
	my $pop_client = new PopClient( $server->popPort() , $System::localhost ) ;
	$pop_client->open() ;
	$pop_client->login( "me" , "secret" ) ;

	# test that the server sees the client disconnect
	System::waitForFileLine( $server->stderr() , "pop connection from" ) ;
	$pop_client->disconnect() ;
	System::waitForFileLine( $server->stderr() , "pop connection closed" ) ;

	# tear down
	$server->kill() ;
	$server->cleanup() ;
}

sub testServerFlushNoMessages
{
	# setup
	my %args = (
		Log => 1 ,
		Verbose => 1 ,
		AsServer => 1 ,
		Domain => 1 ,
		Port => 1 ,
		Admin => 1 ,
		SpoolDir => 1 ,
		ForwardTo => 1 ,
		PidFile => 1 ,
	) ;
	my $spool_dir = System::createSpoolDir() ;
	my $server = new Server(undef,undef,undef,$spool_dir) ;
	Check::ok( $server->run(\%args) , "failed to run" , $server->message() ) ;
	Check::running( $server->pid() , $server->message() ) ;

	# test for an appropriate protocol response if nothing to send
	my $admin_client = new AdminClient( $server->adminPort() , $System::localhost ) ;
	Check::ok( $admin_client->open() ) ;
	$admin_client->doFlush() ;
	my $line = $admin_client->getline() ;
	Check::that( $line eq "error: no messages to send" , "unexpected response" , $line ) ;

	# tear down
	$server->kill() ;
	$server->cleanup() ;
}

sub testServerFlushNoServer
{
	# setup
	my %args = (
		Log => 1 ,
		Verbose => 1 ,
		AsServer => 1 ,
		Domain => 1 ,
		Port => 1 ,
		Admin => 1 ,
		SpoolDir => 1 ,
		ForwardTo => 1 ,
		PidFile => 1 ,
	) ;
	my $spool_dir = System::createSpoolDir() ;
	System::submitSmallMessage( $spool_dir ) ;
	my $server = new Server(undef,undef,undef,$spool_dir) ;
	$server->run(\%args) ;
	Check::running( $server->pid() , $server->message() ) ;
	Check::fileMatchCount( $spool_dir ."/emailrelay.*.content", 1 ) ;
	Check::fileMatchCount( $spool_dir ."/emailrelay.*.envelope", 1 ) ;

	# test for an appropriate admin response and files unaffected if cannot connect on the flush command
	my $admin_client = new AdminClient( $server->adminPort() , $System::localhost ) ;
	Check::ok( $admin_client->open() ) ;
	$admin_client->doFlush() ;
	my $line = $admin_client->getline() ;
	Check::match( $line , "^error: dns error: no such host" , "unexpected response" ) ;
	Check::fileMatchCount( $spool_dir ."/emailrelay.*.envelope", 1 ) ;

	# tear down
	$server->kill() ;
	$server->cleanup() ;
}

sub testServerFlush
{
	# setup
	my %args = (
		Log => 1 ,
		Verbose => 1 ,
		Domain => 1 ,
		Port => 1 ,
		Admin => 1 ,
		SpoolDir => 1 ,
		ForwardTo => 1 ,
		PidFile => 1 ,
	) ;
	my $spool_dir_1 = System::createSpoolDir( "spool-1" ) ;
	my $spool_dir_2 = System::createSpoolDir( "spool-2" ) ;
	my $server_1 = new Server(undef,undef,undef,$spool_dir_1) ;
	my $server_2 = new Server(System::nextPort(),undef,System::nextPort(),$spool_dir_2) ;
	$server_1->set_dst( $server_2->smtpPort() ) ;
	System::submitMessage( $spool_dir_1 , 10000 ) ;
	System::submitMessage( $spool_dir_1 , 10000 ) ;
	Check::ok( $server_1->run(\%args) , "failed to run" , $server_1->message() ) ;
	Check::ok( $server_2->run(\%args) , "failed to run" , $server_2->message() ) ;
	Check::running( $server_1->pid() , $server_1->message() ) ;
	Check::running( $server_2->pid() , $server_2->message() ) ;
	my $admin_client = new AdminClient( $server_1->adminPort() , $System::localhost ) ;
	Check::ok( $admin_client->open() ) ;
	Check::fileMatchCount( $spool_dir_1 ."/emailrelay.*.content", 2 ) ;
	Check::fileMatchCount( $spool_dir_1 ."/emailrelay.*.envelope", 2 ) ;
	Check::fileMatchCount( $spool_dir_2 ."/emailrelay.*.content", 0 ) ;
	Check::fileMatchCount( $spool_dir_2 ."/emailrelay.*.envelope", 0 ) ;

	# test that messages are sent from an admin flush command
	$admin_client->doFlush() ;
	my $line = $admin_client->getline( 30 ) ;
	Check::that( $line eq "OK" , "unexpected response" , $line ) ;
	Check::fileMatchCount( $spool_dir_1 ."/emailrelay.*.content", 0 ) ;
	Check::fileMatchCount( $spool_dir_1 ."/emailrelay.*.envelope", 0 ) ;
	Check::fileMatchCount( $spool_dir_2 ."/emailrelay.*.content", 2 ) ;
	Check::fileMatchCount( $spool_dir_2 ."/emailrelay.*.envelope", 2 ) ;

	# tear down
	$server_1->kill() ;
	$server_2->kill() ;
	$server_1->cleanup() ;
	$server_2->cleanup() ;
	System::deleteSpoolDir( $spool_dir_1 ) ;
	System::deleteSpoolDir( $spool_dir_2 ) ;
}

sub testServerPolling
{
	# setup
	my %args = (
		Log => 1 ,
		Verbose => 1 ,
		Domain => 1 ,
		Port => 1 ,
		Admin => 1 ,
		SpoolDir => 1 ,
		ForwardTo => 1 ,
		PidFile => 1 ,
		Poll => 1 ,
		ConnectionTimeout => 1 ,
	) ;
	my $spool_dir_1 = System::createSpoolDir( "spool-1" ) ;
	my $spool_dir_2 = System::createSpoolDir( "spool-2" ) ;
	my $server_1 = new Server(undef,undef,undef,$spool_dir_1) ;
	my $server_2 = new Server(System::nextPort(),undef,System::nextPort(),$spool_dir_2) ;
	$server_1->set_dst( $server_2->smtpPort() ) ;
	System::submitMessage( $spool_dir_1 , 10000 ) ;
	$server_1->run(\%args) ;
	$server_2->run(\%args) ;
	Check::running( $server_1->pid() , $server_1->message() ) ;
	Check::running( $server_2->pid() , $server_2->message() ) ;
	my $admin_client = new AdminClient( $server_1->adminPort() , $System::localhost ) ;
	Check::ok( $admin_client->open() ) ;

	# test that the message gets forwarded
	Check::ok( System::drain($server_1->spoolDir()) , "message not forwarded" ) ;
	Check::fileMatchCount( $spool_dir_1 ."/emailrelay.*.content", 0 ) ;
	Check::fileMatchCount( $spool_dir_2 ."/emailrelay.*.content", 1 ) ;

	# tear down
	$server_1->kill() ;
	$server_2->kill() ;
	$server_1->cleanup() ;
	$server_2->cleanup() ;
	System::deleteSpoolDir($spool_dir_1) ;
	System::deleteSpoolDir($spool_dir_2) ;
}

sub testServerWithBadClient
{
	# setup
	my %args = (
		Log => 1 ,
		Verbose => 1 ,
		Domain => 1 ,
		Port => 1 ,
		Admin => 1 ,
		SpoolDir => 1 ,
		PidFile => 1 ,
	) ;
	my $server = new Server() ;
	Check::ok( $server->run(\%args) , "failed to run" , $server->message() ) ;
	Check::running( $server->pid() , $server->message() ) ;

	# test that the server drops the connection if we mess up the client protocol
	my $smtp_client = new SmtpClient( $server->smtpPort() , $System::localhost ) ;
	Check::ok( $smtp_client->open() ) ;
	$smtp_client->doBadHelo() ;
	my $seen_reset = 0 ;
	for( my $i = 0 ; $i < 10 ; $i++ )
	{
		my $error = $smtp_client->doBadCommand() ;
		my $was_reset =
			( $error =~ m/send:/ ) ||
			( $error =~ m/recv:/ ) ;
		$seen_reset = $seen_reset || $was_reset ;
		System::sleep_cs( 20 ) ;
	}
	Check::that( $seen_reset , "connection not dropped" ) ;

	# tear down
	$server->kill() ;
	$server->cleanup() ;
}

sub testServerSizeLimit
{
	# setup
	my %args = (
		Log => 1 ,
		Verbose => 1 ,
		Domain => 1 ,
		Port => 1 ,
		SpoolDir => 1 ,
		PidFile => 1 ,
		MaxSize => 1 ,
	) ;
	my $server = new Server() ;
	Check::ok( $server->run(\%args) , "failed to run" , $server->message() ) ;
	Check::running( $server->pid() , $server->message() ) ;
	my $smtp_client = new SmtpClient( $server->smtpPort() , $System::localhost ) ;
	Check::ok( $smtp_client->open() ) ;
	my $line = "0123456789 123456789 123456789 123456789 123456789" ;
	$line .= $line ;

	# test that if the server rejects an oversized message
	$smtp_client->submit_start() ;
	for( my $i = 0 ; $i < 12 ; $i++ )
	{
		$smtp_client->submit_line( $line ) ;
	}
	my $rsp = $smtp_client->submit_end( undef , 1 ) ; # expect 552
	Check::that( $rsp =~ m/^552 / , "large message submission did not fail as it should have" ) ;
	Check::fileMatchCount( $server->spoolDir()."/emailrelay.*.content" , 0 ) ;
	Check::fileMatchCount( $server->spoolDir()."/emailrelay.*.envelope*" , 0 ) ;
	Check::fileContains( $server->stderr() , "552 message size exceeds fixed maximum message size" ) ;

	# tear down
	$server->kill() ;
	$server->cleanup() ;
}

sub testClientContinuesIfNoSecrets
{
	# setup
	my %server_args = (
		Log => 1 ,
		Verbose => 1 ,
		Domain => 1 ,
		Port => 1 ,
		SpoolDir => 1 ,
		PidFile => 1 ,
		ServerSecrets => 1 ,
	) ;
	my %client_args = (
		Log => 1 ,
		Verbose => 1 ,
		Domain => 1 ,
		SpoolDir => 1 ,
		Verbose => 1 ,
		Forward => 1 ,
		ForwardTo => 1 ,
		DontServe => 1 ,
		NoDaemon => 1 ,
		Hidden => 1 ,
	) ;
	my $server = new Server() ;
	my $client = new Server() ;
	$client->set_dst( $server->smtpPort() ) ;
	System::createFile( $server->serverSecrets() , "server login me secret" ) ;
	System::submitSmallMessage( $client->spoolDir() ) ;
	Check::ok( $server->run(\%server_args) , "failed to run server" , $server->message() ) ;
	System::waitForFile( $server->stderr() ) ;

	# test that the client gets as far as issuing the mail command
	Check::ok( $client->run(\%client_args) , "failed to run client" , $client->message() ) ;
	System::waitForFileLine( $server->stderr() , "MAIL FROM:" ) ;
	System::waitForFileLine( $server->stderr() , "530 authentication required" ) ;

	# tear down
	$server->kill() ;
	$server->cleanup() ;
	$client->cleanup() ;
}

sub testClientSavesReasonCode
{
	# setup
	my %args = (
		Log => 1 ,
		Verbose => 1 ,
		Domain => 1 ,
		SpoolDir => 1 ,
		ForwardTo => 1 ,
		Forward => 1 ,
		DontServe => 1 ,
		NoDaemon => 1 ,
	) ;
	my $test_server = new TestServer( System::nextPort() ) ;
	my $client = new Server() ;
	$client->set_dst( $test_server->port() ) ;
	$test_server->run( "--fail-at 1" ) ;
	System::submitSmallMessage( $client->spoolDir() ) ;
	System::submitSmallMessage( $client->spoolDir() ) ;

	# test that the client runs but the failed message envelope has a reason code
	Check::ok( $client->run(\%args) , "failed to run as client" ) ;
	System::waitForFiles( $client->spoolDir()."/emailrelay.*.envelope*bad" , 1 , "envelope" ) ;
	Check::fileMatchCount( $client->spoolDir()."/emailrelay.*.content" , 1 , "content" ) ;
	my @files = System::glob_( $client->spoolDir()."/emailrelay*bad" ) ;
	Check::fileContains( $files[0] , "X-MailRelay-ReasonCode: 452" ) ;

	# tear down
	$client->wait() ;
	$test_server->kill() ;
	$client->cleanup() ;
	$test_server->cleanup() ;
}

sub testFilter
{
	# setup
	my %args = (
		Log => 1 ,
		Verbose => 1 ,
		Domain => 1 ,
		Port => 1 ,
		SpoolDir => 1 ,
		PidFile => 1 ,
		Filter => 1 ,
	) ;
	my $server = new Server() ;
	my $outputfile = System::tempfile( "output" ) ;
	Filter::create( $server->filter() , {edit=>1} , {
			unix => [
				"echo \"\$\@\" | sed 's/^/arg: /' > $outputfile" ,
				"env | sed 's/^/env: /' >> $outputfile" ,
				"exit 0" ,
			] ,
			win32 => [
				"var fs = WScript.CreateObject(\"Scripting.FileSystemObject\");" ,
				"var out_ = fs.OpenTextFile( \"$outputfile\" , 8 , true ) ;" ,
				"out_.WriteLine( \"arg: \" + WScript.Arguments(0) ) ;" ,
				"for( var i = 0 ; i < 1000 ; i++ ) {" ,
				" WScript.Stdout.WriteLine( \"test filling up the pipe with junk\" ) ;" ,
				"}" ,
				"WScript.Quit(0);" ,
			] ,
		} ) ;
	Check::fileExists( $server->filter() ) ;
	Check::ok( $server->run(\%args) , "failed to run" , $server->message() ) ;
	Check::running( $server->pid() , $server->message() ) ;
	my $smtp_client = new SmtpClient( $server->smtpPort() , $System::localhost ) ;
	Check::ok( $smtp_client->open() ) ;

	# test that the filter edits the envelope and is executed with the correct environment
	$smtp_client->submit() ;
	Check::that( -f $outputfile , "filter did not create an output file" ) ;
	Check::fileContains( $outputfile , "arg: " , "filter did not generate output" ) ;
	Check::fileContains( $outputfile , "arg: /" , "filter not passed an absolute path" ) if System::unix() ;
	Check::fileLineCountLessThan( $outputfile , 7 , "env: " , "wrong number of environment variables" ) if System::unix() ;
	Check::fileMatchCount( $server->spoolDir()."/emailrelay.*.content" , 1 ) ;
	Check::fileMatchCount( $server->spoolDir()."/emailrelay.*.envelope" , 1 ) ;
	Check::fileContains( System::match($server->spoolDir()."/emailrelay.*.envelope") , "FROM-EDIT" ) ;

	# tear down
	System::unlink( $outputfile ) ;
	$server->kill() ;
	$server->cleanup() ;
}

sub testFilterIdentity
{
	# setup
	my %args = (
		Log => 1 ,
		Verbose => 1 ,
		Domain => 1 ,
		Port => 1 ,
		SpoolDir => 1 ,
		PidFile => 1 ,
		Filter => 1 ,
		User => 1 ,
	) ;
	requireUnix() ;
	requireRoot() ;
	my $server = new Server() ;
	my $outputfile = System::tempfile( "output" ) ;
	Filter::create( $server->filter() , {} , {
			unix => [
				'ps -p $$'." -o ruid,uid | sed 's/  */ /g' | sed 's/^ *//' > $outputfile" ,
				'grep Uid: /proc/$$/status | perl -ane "print @F[1],chr(32),@F[2],chr(10)" >> '."$outputfile" ,
				"exit 0" ,
			] ,
			win32 => [
				# not used
			] ,
		} ) ;
	Check::ok( $server->run(\%args) , "failed to run" , $server->message() ) ;
	Check::running( $server->pid() , $server->message() ) ;
	my $smtp_client = new SmtpClient( $server->smtpPort() , $System::localhost ) ;
	Check::ok( $smtp_client->open() ) ;

	# test that the filter is executed with the correct identity
	$smtp_client->submit() ;
	Check::that( -f $outputfile , "filter did not create an output file" ) ;
	my $uid1 = System::uid( $server->user() ) ;
	my $uid2 = $uid1 - 4294967296 ; # negative ids on mac
	Check::fileContainsEither( $outputfile , "$uid1 $uid1" , "$uid2 $uid2" , "filter not run as uid $uid1" ) ;

	# tear down
	System::unlink( $outputfile ) ;
	$server->kill() ;
	$server->cleanup() ;
}

sub testFilterFailure
{
	# setup
	my %args = (
		Log => 1 ,
		Verbose => 1 ,
		Domain => 1 ,
		Port => 1 ,
		SpoolDir => 1 ,
		PidFile => 1 ,
		Filter => 1 ,
	) ;
	my $server = new Server() ;
	Filter::create( $server->filter() , {} ,{
			unix => [
				"echo aaa" ,
				"echo '<<foo bar>> yy'" ,
				"echo zzz" ,
				"exit 3" ,
			] ,
			win32 => [
				"WScript.StdOut.WriteLine(\"aaa\");" ,
				"WScript.StdOut.WriteLine(\"<<foo bar>> yy\");" ,
				"WScript.StdOut.WriteLine(\"zzz\");" ,
				"WScript.Quit(3);" ,
			] ,
		} ) ;
	Check::ok( $server->run(\%args) , "failed to run" , $server->message() ) ;
	Check::running( $server->pid() , $server->message() ) ;
	my $smtp_client = new SmtpClient( $server->smtpPort() , $System::localhost ) ;
	Check::ok( $smtp_client->open() ) ;

	# test that if the filter rejects the message then the submit fails and no files are spooled
	$smtp_client->submit( 1 ) ;
	Check::fileMatchCount( $server->spoolDir()."/emailrelay.*.content" , 0 ) ;
	Check::fileMatchCount( $server->spoolDir()."/emailrelay.*.envelope*" , 0 ) ;
	Check::fileContains( $server->stderr() , "rejected by filter: .foo bar." ) ;

	# tear down
	$server->kill() ;
	$server->cleanup() ;
}

sub testFilterTimeout
{
	# setup
	my %args = (
		Log => 1 ,
		Verbose => 1 ,
		Domain => 1 ,
		Port => 1 ,
		SpoolDir => 1 ,
		PidFile => 1 ,
		Filter => 1 ,
		FilterTimeout => 1 ,
	) ;
	my $server = new Server() ;
	Filter::create( $server->filter() , {} , {
			unix => [
				"sleep 3" ,
				"exit 0" ,
			] ,
			win32 => [
				"WScript.Sleep(3000) ;" ,
				"WScript.Quit(0) ;" ,
			] ,
		} ) ;
	Check::ok( $server->run(\%args) , "failed to run" , $server->message() ) ;
	Check::running( $server->pid() , $server->message() ) ;
	my $smtp_client = new SmtpClient( $server->smtpPort() , $System::localhost ) ;
	Check::ok( $smtp_client->open() ) ;

	# test that the filter times out
	$smtp_client->submit( 1 ) ;
	Check::fileMatchCount( $server->spoolDir()."/emailrelay.*.content" , 0 ) ;
	Check::fileMatchCount( $server->spoolDir()."/emailrelay.*.envelope*" , 0 ) ;
	Check::fileContains( $server->stderr() , "filter done: ok=0 response=.error. reason=.*timeout" ) ;

	# tear down
	$server->kill() ;
	$server->cleanup() ;
}

sub testFilterWithBadFileDeletion
{
	_testFilterWithFileDeletion(0,1) ;
}

sub testFilterWithGoodFileDeletion
{
	_testFilterWithFileDeletion(100,0) ;
}

sub _testFilterWithFileDeletion
{
	my ( $exit_code , $expect_submit_error ) = @_ ;

	# setup
	my %args = (
		Log => 1 ,
		Verbose => 1 ,
		Domain => 1 ,
		Port => 1 ,
		SpoolDir => 1 ,
		PidFile => 1 ,
		Filter => 1 , # proxy
		ForwardTo => 1 , # proxy
		Immediate => 1 , # proxy
	) ;
	my $server_1 = new Server() ; # proxy
	my $server_2 = new Server( System::nextPort() ) ; # target server
	$server_1->set_dst( $server_2->smtpPort() ) ;
	Filter::create( $server_1->filter() , {} , {
			unix => [
				'rm `dirname $1`/emailrelay.*' ,
				"exit $exit_code" ,
			] ,
			win32 => [
				"var content = WScript.Arguments(0) ;" ,
				"var spool_dir = content.substr(0,content.lastIndexOf(\"\\\\\")) ;" ,
				"var fs = WScript.CreateObject(\"Scripting.FileSystemObject\") ;" ,
				"fs.DeleteFile( spool_dir + \"\\\\emailrelay.*\" ) ;" ,
				"WScript.Quit($exit_code);" ,
			] ,
		} ) ;
	Check::ok( $server_1->run(\%args) , "failed to run" , $server_1->message() ) ;
	delete $args{Filter} ;
	delete $args{ForwardTo} ;
	delete $args{Immediate} ;
	Check::ok( $server_2->run(\%args) , "failed to run" , $server_2->rc() ) ;
	Check::running( $server_1->pid() , $server_1->message() ) ;
	Check::running( $server_2->pid() , $server_2->message() ) ;
	my $smtp_client = new SmtpClient( $server_1->smtpPort() , $System::localhost ) ;
	Check::ok( $smtp_client->open() ) ;

	# test that once the filter deletes the message files then proxying succeeds or fails depending on the exit code
	$smtp_client->submit($expect_submit_error) ;

	# tear down
	$server_1->kill() ;
	$server_2->kill() ;
	$server_1->cleanup() ;
	$server_2->cleanup() ;
	System::deleteSpoolDir( $server_1->spoolDir() ) ;
	System::deleteSpoolDir( $server_2->spoolDir() ) ;
}

sub testFilterRescan
{
	# setup
	my %args = (
		Log => 1 ,
		Verbose => 1 ,
		Domain => 1 ,
		Port => 1 ,
		SpoolDir => 1 ,
		PidFile => 1 ,
		Filter => 1 ,
		ForwardTo => 1 ,
	) ;
	my $server = new Server() ;
	Filter::create( $server->filter() , {} , {
			unix => [
				"exit 103" ,
			] ,
			win32 => [
				"WScript.Quit(103) ;" ,
			] ,
		} ) ;
	Check::ok( $server->run(\%args) , "failed to run" , $server->message() ) ;
	Check::running( $server->pid() , $server->message() ) ;
	my $smtp_client = new SmtpClient( $server->smtpPort() , $System::localhost ) ;
	Check::ok( $smtp_client->open() ) ;

	# test that the rescan is triggered by the filter exit code
	$smtp_client->submit() ;
	System::waitForFileLine( $server->stderr() , "forwarding: .rescan" , "no rescan message in the log file" ) ;

	# tear down
	$server->kill() ;
	$server->cleanup() ;
}

sub testFilterParallelism
{
	# setup
	my %args = (
		Log => 1 ,
		Verbose => 1 ,
		Domain => 1 ,
		Port => 1 ,
		SpoolDir => 1 ,
		PidFile => 1 ,
		Filter => 1 ,
		Poll => 1 ,
		ForwardTo => 1 ,
	) ;
	requireThreads() ;
	my $server = new Server() ;
	Filter::create( $server->filter() , {} , {
			unix => [
				"sleep 3" ,
				"exit 0" ,
			] ,
			win32 => [
				"WScript.Sleep(3000) ;" ,
				"WScript.Quit(0) ;" ,
			] ,
		} ) ;
	Check::ok( $server->run(\%args) , "failed to run" , $server->message() ) ;
	Check::running( $server->pid() , $server->message() ) ;
	my $smtp_client_1 = new SmtpClient( $server->smtpPort() , $System::localhost ) ;
	my $smtp_client_2 = new SmtpClient( $server->smtpPort() , $System::localhost ) ;
	Check::ok( $smtp_client_1->open() ) ;
	Check::ok( $smtp_client_2->open() ) ;
	$smtp_client_1->submit_start() ;
	$smtp_client_2->submit_start() ;
	$smtp_client_1->submit_line() ;
	$smtp_client_2->submit_line() ;
	$smtp_client_1->submit_end_nowait() ;
	$smtp_client_2->submit_end_nowait() ;

	# test that the two messages commit at roughly the same time
	sleep( 1 ) ;
	Check::fileMatchCount( $server->spoolDir()."/emailrelay.*.envelope" , 0 ) ;
	sleep( 3 ) ;
	Check::fileMatchCount( $server->spoolDir()."/emailrelay.*.envelope" , 2 ) ;

	# tear down
	$server->kill() ;
	$server->cleanup() ;
}

sub testScannerPass
{
	# setup
	my %args = (
		Log => 1 ,
		Verbose => 1 ,
		Domain => 1 ,
		Port => 1 ,
		SpoolDir => 1 ,
		PidFile => 1 ,
		Scanner => 1 ,
	) ;
	my $server = new Server() ;
	my $scanner = new Scanner( $server->scannerAddress() ) ;
	Check::ok( $server->run(\%args) , "failed to run" , $server->message() ) ;
	Check::running( $server->pid() , $server->message() ) ;
	$scanner->run() ;
	my $smtp_client = new SmtpClient( $server->smtpPort() , $System::localhost ) ;
	Check::ok( $smtp_client->open() ) ;

	# test that the scanner is used
	$smtp_client->submit_start() ;
	$smtp_client->submit_line( "send ok" ) ; # (the test scanner treats the message body as a script)
	$smtp_client->submit_end() ;
	Check::fileContains( $scanner->logfile() , "new connection from" ) ;
	Check::fileContains( $scanner->logfile() , "send ok" ) ;
	Check::fileDoesNotContain( $server->stderr() , "452 " ) ;
	Check::fileMatchCount( $server->spoolDir()."/emailrelay.*.envelope" , 1 ) ;

	# tear down
	$server->kill() ;
	$scanner->kill() ;
	$scanner->cleanup() ;
	$server->cleanup() ;
}

sub testScannerBlock
{
	# setup
	my %args = (
		Log => 1 ,
		Verbose => 1 ,
		Domain => 1 ,
		Port => 1 ,
		SpoolDir => 1 ,
		PidFile => 1 ,
		Scanner => 1 ,
	) ;
	my $server = new Server() ;
	my $scanner = new Scanner( $server->scannerAddress() ) ;
	Check::ok( $server->run(\%args) , "failed to run" , $server->message() ) ;
	Check::running( $server->pid() , $server->message() ) ;
	$scanner->run() ;
	my $smtp_client = new SmtpClient( $server->smtpPort() , $System::localhost ) ;
	Check::ok( $smtp_client->open() ) ;

	# test that the scanner is used
	$smtp_client->submit_start() ;
	$smtp_client->submit_line( "send foobar" ) ; # (the test scanner treats the message body as a script)
	$smtp_client->submit_end( 1 ) ; # 1 <= expect the "." command to fail
	Check::fileContains( $server->stderr() , "rejected by filter: .foobar" ) ;
	Check::fileContains( $server->stderr() , "452 foobar" ) ;
	Check::fileMatchCount( $server->spoolDir()."/emailrelay.*.envelope" , 0 ) ;

	# tear down
	$server->kill() ;
	$scanner->kill() ;
	$scanner->cleanup() ;
	$server->cleanup() ;
}

sub testScannerTimeout
{
	# setup
	my %args = (
		Log => 1 ,
		Verbose => 1 ,
		Domain => 1 ,
		Port => 1 ,
		SpoolDir => 1 ,
		PidFile => 1 ,
		Scanner => 1 ,
		FilterTimeout => 1 ,
	) ;
	my $server = new Server() ;
	my $scanner = new Scanner( $server->scannerAddress() ) ;
	Check::ok( $server->run(\%args) , "failed to run" , $server->message() ) ;
	Check::running( $server->pid() , $server->message() ) ;
	$scanner->run() ;
	my $smtp_client = new SmtpClient( $server->smtpPort() , $System::localhost ) ;
	Check::ok( $smtp_client->open() ) ;

	# test that the scanner times out
	$smtp_client->submit_start() ;
	$smtp_client->submit_line( "sleep 3" ) ;
	$smtp_client->submit_line( "send foobar" ) ;
	$smtp_client->submit_end( 1 ) ;
	Check::fileDoesNotContain( $server->stderr() , "452 foobar" ) ;
	Check::fileContains( $server->stderr() , "filter done: ok=0 response=.failed. reason=.*timeout" ) ;
	Check::fileContains( $server->stderr() , "452 failed" ) ;

	# tear down
	$server->kill() ;
	$scanner->kill() ;
	$scanner->cleanup() ;
	$server->cleanup() ;
}

sub testScannerOverUnixDomainSockets
{
	# setup
	my %args = (
		Log => 1 ,
		Verbose => 1 ,
		Domain => 1 ,
		Port => 1 ,
		SpoolDir => 1 ,
		PidFile => 1 ,
		Scanner => 1 ,
	) ;
	requireUnixDomainSockets() ;
	my $server = new Server() ;
	$server->set_scannerAddress( System::tempfile() ) ;
	my $scanner = new Scanner( $server->scannerAddress() ) ;
	Check::ok( $server->run(\%args) , "failed to run" , $server->message() ) ;
	Check::running( $server->pid() , $server->message() ) ;
	$scanner->run() ;
	my $smtp_client = new SmtpClient( $server->smtpPort() , $System::localhost ) ;
	Check::ok( $smtp_client->open() ) ;

	# test that the scanner is used
	$smtp_client->submit_start() ;
	$smtp_client->submit_line( "send ok" ) ; # (the test scanner treats the message body as a script)
	$smtp_client->submit_end() ;
	Check::fileContains( $scanner->logfile() , "new connection from (/|\\\\0)" ) ;
	Check::fileContains( $scanner->logfile() , "send ok" ) ;
	Check::fileDoesNotContain( $server->stderr() , "452 " ) ;
	Check::fileMatchCount( $server->spoolDir()."/emailrelay.*.envelope" , 1 ) ;

	# tear down
	$server->kill() ;
	$scanner->kill() ;
	$scanner->cleanup() ;
	$server->cleanup() ;
}

sub testNetworkVerifierPass
{
	# setup
	my %args = (
		Log => 1 ,
		Verbose => 1 ,
		Domain => 1 ,
		Port => 1 ,
		SpoolDir => 1 ,
		PidFile => 1 ,
		Verifier => 1 ,
	) ;
	my $server = new Server() ;
	my $verifier = new Verifier( $server->verifierPort() ) ;
	Check::ok( $server->run(\%args) , "failed to run" , $server->message() ) ;
	Check::running( $server->pid() , $server->message() ) ;
	$verifier->run() ;
	my $smtp_client = new SmtpClient( $server->smtpPort() , $System::localhost ) ;
	Check::ok( $smtp_client->open() ) ;

	# test that the verifier is used
	$smtp_client->submit_start( "OK\@here" ) ; # (the test verifier interprets the recipient string)
	$smtp_client->submit_line( "just testing" ) ;
	$smtp_client->submit_end() ;
	Check::fileContains( $verifier->logfile() , "sending valid" ) ;
	Check::fileDoesNotContain( $server->stderr() , "452 " ) ;
	Check::fileMatchCount( $server->spoolDir()."/emailrelay.*.envelope" , 1 ) ;

	# tear down
	$server->kill() ;
	$verifier->kill() ;
	$verifier->cleanup() ;
	$server->cleanup() ;
}

sub testNetworkVerifierFail
{
	# setup
	my %args = (
		Log => 1 ,
		Verbose => 1 ,
		Domain => 1 ,
		Port => 1 ,
		SpoolDir => 1 ,
		PidFile => 1 ,
		Verifier => 1 ,
	) ;
	my $server = new Server() ;
	my $verifier = new Verifier( $server->verifierPort() ) ;
	Check::ok( $server->run(\%args) , "failed to run" , $server->message() ) ;
	Check::running( $server->pid() , $server->message() ) ;
	$verifier->run() ;
	my $smtp_client = new SmtpClient( $server->smtpPort() , $System::localhost ) ;
	Check::ok( $smtp_client->open() ) ;

	# test that the verifier can reject
	$smtp_client->submit_start( "fail\@here" , 1 ) ; # (the test verifier interprets the recipient string)
	Check::fileContains( $verifier->logfile() , "sending error" ) ;
	Check::fileContains( $server->stderr() , "VerifierError" ) ; # see emailrelay_test_verifier.cpp
	Check::fileMatchCount( $server->spoolDir()."/emailrelay.*.envelope" , 0 ) ;

	# tear down
	$server->kill() ;
	$verifier->kill() ;
	$verifier->cleanup() ;
	$server->cleanup() ;
}

sub testProxyConnectsOnce
{
	# setup
	my %args = (
		Log => 1 ,
		Verbose => 1 ,
		Domain => 1 ,
		Port => 1 ,
		Admin => 1 ,
		SpoolDir => 1 ,
		ForwardTo => 1 ,
		Immediate => 1 ,
		PidFile => 1 ,
	) ;
	my $spool_dir_1 = System::createSpoolDir( "spool-1" ) ;
	my $spool_dir_2 = System::createSpoolDir( "spool-2" ) ;
	my $server_1 = new Server(undef,undef,undef,$spool_dir_1) ;
	my $server_2 = new Server(System::nextPort,undef,System::nextPort(),$spool_dir_2) ;
	$server_1->set_dst( $server_2->smtpPort() ) ;
	Check::ok( $server_1->run(\%args) , "failed to run" , $server_1->message() ) ;
	delete $args{Immediate} ;
	delete $args{ForwardTo} ;
	Check::ok( $server_2->run(\%args) , "failed to run" , $server_2->message() ) ;
	Check::running( $server_1->pid() , $server_1->message() ) ;
	Check::running( $server_2->pid() , $server_2->message() ) ;
	my $smtp_client = new SmtpClient( $server_1->smtpPort() , $System::localhost ) ;
	Check::ok( $smtp_client->open() ) ;
	Check::fileMatchCount( $spool_dir_1 ."/emailrelay.*", 0 ) ;
	Check::fileMatchCount( $spool_dir_2 ."/emailrelay.*", 0 ) ;

	# test that the proxy uses one connection to forward multiple messages
	my $n = 4 ;
	for( my $i = 0 ; $i < $n ; $i++ )
	{
		$smtp_client->submit_start() ;
		$smtp_client->submit_line( "foo bar" ) ;
		$smtp_client->submit_end() ;
	}
	$smtp_client->close() ;
	System::waitForFileLine( $server_2->stderr() , "smtp connection closed" ) ;
	Check::fileMatchCount( $spool_dir_1 ."/emailrelay.*", 0 ) ;
	Check::fileMatchCount( $spool_dir_2 ."/emailrelay.*.envelope", $n ) ;
	Check::fileMatchCount( $spool_dir_2 ."/emailrelay.*.content", $n ) ;
	Check::fileLineCount(  $server_2->stderr() , 1 , "smtp connection from" ) ;
	Check::fileLineCount(  $server_2->stderr() , 1 , "smtp connection closed" ) ;

	# tear down
	$server_1->kill() ;
	$server_2->kill() ;
	$server_1->cleanup() ;
	$server_2->cleanup() ;
	System::deleteSpoolDir( $spool_dir_1 ) ;
	System::deleteSpoolDir( $spool_dir_2 ) ;
}

sub testClientFilterPass
{
	# setup
	my %args = (
		Log => 1 ,
		Verbose => 1 ,
		Domain => 1 ,
		SpoolDir => 1 ,
		Forward => 1 ,
		ForwardTo => 1 ,
		ClientFilter => 1 ,
		DontServe => 1 ,
		NoDaemon => 1 ,
	) ;
	my $spool_dir_1 = System::createSpoolDir( "spool-1" ) ;
	my $spool_dir_2 = System::createSpoolDir( "spool-2" ) ;
	System::submitSmallMessage( $spool_dir_1 ) ;
	System::submitSmallMessage( $spool_dir_1 ) ;
	Check::fileMatchCount( $spool_dir_1 ."/emailrelay.*.envelope", 2 ) ;
	my $server_1 = new Server(undef,undef,undef,$spool_dir_1) ;
	my $server_2 = new Server(System::nextPort(),undef,System::nextPort(),$spool_dir_2) ;
	my %server_args = %args ;
	delete $server_args{Forward} ;
	delete $server_args{ForwardTo} ;
	delete $server_args{ClientFilter} ;
	delete $server_args{DontServe} ;
	delete $server_args{NoDaemon} ;
	$server_args{PidFile} = 1 ;
	$server_args{Port} = 1 ;
	Check::ok( $server_2->run(\%server_args) , "failed to run" , $server_2->message() ) ;
	Check::running( $server_2->pid() , $server_2->message() ) ;
	$server_1->set_dst( $server_2->smtpPort() ) ;
	my $outputfile = System::tempfile( "output" ) ;
	Filter::create( $server_1->clientFilter() , {edit=>1} , {
			unix => [
				"echo \"\$\@\" | sed 's/^/arg: /' > $outputfile" ,
				"env | sed 's/^/env: /' >> $outputfile" ,
				"exit 0" ,
			] ,
			win32 => [
				"var fs = WScript.CreateObject(\"Scripting.FileSystemObject\");" ,
				"var out_ = fs.OpenTextFile( \"$outputfile\" , 8 , true ) ;" ,
				"out_.WriteLine( \"arg: \" + WScript.Arguments(0) ) ;" ,
				"WScript.Quit(0) ;" ,
			] ,
		} ) ;
	Check::fileExists( $server_1->clientFilter() ) ;
	Check::ok( $server_1->run(\%args) , "failed to run" , $server_1->message() ) ;

	# test that the client filter runs and the messages are edited and forwarded
	System::waitForFiles( $spool_dir_2 ."/emailrelay.*" , 4 ) ;
	System::waitForFiles( $spool_dir_1 . "/emailrelay.*" , 0 ) ;
	Check::fileExists( $outputfile , "no output file generated by the client filter" ) ;
	Check::fileMatchCount( $spool_dir_2 ."/emailrelay.*.envelope", 2 ) ;
	Check::fileMatchCount( $spool_dir_2 ."/emailrelay.*.content", 2 ) ;
	Check::fileContains( System::matchOne($spool_dir_2."/emailrelay.*.envelope",0,2) , "FROM-EDIT" ) ;
	Check::fileContains( System::matchOne($spool_dir_2."/emailrelay.*.envelope",1,2) , "FROM-EDIT" ) ;

	# tear down
	$server_1->wait() ;
	$server_2->kill() ;
	$server_1->cleanup() ;
	$server_2->cleanup() ;
	System::deleteSpoolDir( $spool_dir_1 ) ;
	System::deleteSpoolDir( $spool_dir_2 ) ;
	System::unlink( $outputfile ) ;
}

sub testClientFilterBlock
{
	# setup
	my %args = (
		Log => 1 ,
		Verbose => 1 ,
		Domain => 1 ,
		SpoolDir => 1 ,
		Forward => 1 ,
		ForwardTo => 1 ,
		ClientFilter => 1 ,
		DontServe => 1 ,
		NoDaemon => 1 ,
		Hidden => 1 ,
	) ;
	my $spool_dir_1 = System::createSpoolDir( "spool-1" ) ;
	my $spool_dir_2 = System::createSpoolDir( "spool-2" ) ;
	System::submitSmallMessage( $spool_dir_1 ) ;
	System::submitSmallMessage( $spool_dir_1 ) ;
	Check::fileMatchCount( $spool_dir_1 ."/emailrelay.*.envelope", 2 ) ;
	my $server_1 = new Server(undef,undef,undef,$spool_dir_1) ;
	my $server_2 = new Server(System::nextPort(),undef,System::nextPort(),$spool_dir_2) ;
	my %server_args = %args ;
	delete $server_args{Forward} ;
	delete $server_args{ForwardTo} ;
	delete $server_args{ClientFilter} ;
	delete $server_args{DontServe} ;
	delete $server_args{NoDaemon} ;
	delete $server_args{Hidden} ;
	$server_args{PidFile} = 1 ;
	$server_args{Port} = 1 ;
	Check::ok( $server_2->run(\%server_args) , "failed to run" , $server_2->message() ) ;
	Check::running( $server_2->pid() , $server_2->message() ) ;
	$server_1->set_dst( $server_2->smtpPort() ) ;
	my $outputfile = System::tempfile( "output" ) ;
	Filter::create( $server_1->clientFilter() , {edit=>1} , {
			unix => [
				"echo '<<foo bar>>sldkfj'" ,
				"echo a > $outputfile" ,
				"exit 13" ,
			] ,
			win32 => [
				"WScript.StdOut.WriteLine(\"<<foo bar>>sldkfj\") ;" ,
				"var fs = WScript.CreateObject(\"Scripting.FileSystemObject\");" ,
				"var out_ = fs.OpenTextFile( \"$outputfile\" , 8 , true ) ;" ,
				"out_.WriteLine(\"a\") ;" ,
				"WScript.Quit(13) ;" ,
			] ,
		} ) ;
	Check::ok( $server_1->run(\%args) , "failed to run" , $server_1->message() ) ;

	# test that the client filter runs and the messages are failed
	System::waitForFiles( $spool_dir_1 ."/emailrelay.*.bad", 2 ) ;
	Check::fileMatchCount( $spool_dir_1 ."/emailrelay.*.content", 2 ) ;
	Check::fileExists( $outputfile , "no output file generated by the client filter" ) ;
	Check::fileMatchCount( $spool_dir_2 ."/emailrelay.*", 0 ) ;

	# tear down
	$server_1->wait() ;
	$server_2->kill() ;
	$server_1->cleanup() ;
	$server_2->cleanup() ;
	System::deleteSpoolDir( $spool_dir_1 , 1 ) ;
	System::deleteSpoolDir( $spool_dir_2 , 1 ) ;
	System::unlink( $outputfile ) ;
}

sub testClientGivenUnknownMechanisms
{
	# setup
	my %args = (
		Log => 1 ,
		Verbose => 1 ,
		Domain => 1 ,
		SpoolDir => 1 ,
		Forward => 1 ,
		ForwardTo => 1 ,
		DontServe => 1 ,
		NoDaemon => 1 ,
		ClientAuth => 1 ,
		Hidden => 1 ,
	) ;
	my $spool_dir = System::createSpoolDir() ;
	my $test_server = new TestServer( System::nextPort() ) ;
	$test_server->run( "--auth-foo-bar" ) ;
	System::submitSmallMessage( $spool_dir ) ;
	System::submitSmallMessage( $spool_dir ) ;
	Check::fileMatchCount( $spool_dir ."/emailrelay.*.envelope", 2 ) ;
	my $emailrelay = new Server( undef , undef , undef , $spool_dir ) ;
	System::createFile( $emailrelay->clientSecrets() , "client login me secret" ) ;
	$emailrelay->set_dst( $test_server->port() ) ;

	# test that protocol fails and one message fails
	Check::ok( $emailrelay->run( \%args ) , "failed to run" , $emailrelay->message() ) ;
	System::waitForFileLine( $emailrelay->stderr() , "cannot do authentication required by " ) ;
	Check::fileMatchCount( $spool_dir ."/emailrelay.*.envelope", 1 ) ;
	Check::fileMatchCount( $spool_dir ."/emailrelay.*.envelope.bad", 1 ) ;

	# tear down
	$emailrelay->wait() ;
	$test_server->kill() ;
	$emailrelay->cleanup() ;
	$test_server->cleanup() ;
}

sub testClientAuthenticationFailure
{
	# setup
	my %args = (
		Log => 1 ,
		Verbose => 1 ,
		Domain => 1 ,
		SpoolDir => 1 ,
		Forward => 1 ,
		ForwardTo => 1 ,
		DontServe => 1 ,
		NoDaemon => 1 ,
		ClientAuth => 1 ,
		Hidden => 1 ,
	) ;
	my $spool_dir = System::createSpoolDir() ;
	my $test_server = new TestServer( System::nextPort() ) ;
	$test_server->run( "--auth-login" ) ;
	System::submitSmallMessage( $spool_dir ) ;
	System::submitSmallMessage( $spool_dir ) ;
	Check::fileMatchCount( $spool_dir ."/emailrelay.*.envelope", 2 ) ;
	my $emailrelay = new Server( undef , undef , undef , $spool_dir ) ;
	System::createFile( $emailrelay->clientSecrets() , "client login me secret" ) ;
	$emailrelay->set_dst( $test_server->port() ) ;

	# test that protocol fails and one message fails
	Check::ok( $emailrelay->run( \%args ) , "failed to run" , $emailrelay->message() ) ;
	System::waitForFileLine( $emailrelay->stderr() , "authentication failed" ) ;
	System::waitForFiles( $spool_dir ."/emailrelay.*.envelope.bad", 1 ) ;
	Check::fileMatchCount( $spool_dir ."/emailrelay.*.envelope", 1 ) ;

	# tear down
	$test_server->kill() ;
	$emailrelay->wait() ;
	$test_server->cleanup() ;
	$emailrelay->cleanup() ;
}

sub testClientMessageFailure
{
	# setup
	my %args = (
		Log => 1 ,
		Verbose => 1 ,
		Domain => 1 ,
		SpoolDir => 1 ,
		Forward => 1 ,
		ForwardTo => 1 ,
		DontServe => 1 ,
		NoDaemon => 1 ,
		Hidden => 1 ,
	) ;
	my $spool_dir = System::createSpoolDir() ;
	my $test_server = new TestServer( System::nextPort() ) ;
	$test_server->run( "--fail-at 2" ) ;
	System::submitSmallMessage( $spool_dir ) ;
	System::submitSmallMessage( $spool_dir ) ;
	System::submitSmallMessage( $spool_dir ) ;
	System::submitSmallMessage( $spool_dir ) ;
	Check::fileMatchCount( $spool_dir ."/emailrelay.*.envelope", 4 ) ;
	my $emailrelay = new Server( undef , undef , undef , $spool_dir ) ;
	$emailrelay->set_dst( $test_server->port() ) ;

	# test that two of the four messages are left as ".bad"
	Check::ok( $emailrelay->run( \%args ) , "failed to run" , $emailrelay->message() ) ;
	System::waitForFiles( $spool_dir ."/emailrelay.*.bad" , 2 ) ;
	Check::fileMatchCount( $spool_dir ."/emailrelay.*.content", 2 ) ;
	Check::fileMatchCount( $spool_dir ."/emailrelay.*.envelope.bad", 2 ) ;
	Check::fileMatchCount( $spool_dir ."/emailrelay.*.envelope", 0 ) ;

	# tear down
	$test_server->kill() ;
	$emailrelay->wait() ;
	$test_server->cleanup() ;
	$emailrelay->cleanup() ;
}

sub testClientInvalidRecipients
{
	# setup
	my %args = (
		Log => 1 ,
		Verbose => 1 ,
		Domain => 1 ,
		SpoolDir => 1 ,
		Forward => 1 ,
		ForwardTo => 1 ,
		DontServe => 1 ,
		NoDaemon => 1 ,
		Hidden => 1 ,
	) ;
	my $spool_dir = System::createSpoolDir() ;
	my $test_server = new TestServer( System::nextPort() ) ;
	$test_server->run() ;
	System::submitSmallMessage( $spool_dir , "acceptme\@there.com" ) ;
	System::submitSmallMessage( $spool_dir , "acceptme1\@there.com" , "acceptme2\@there.com" ) ;
	System::submitSmallMessage( $spool_dir , "acceptme\@there.com" , "rejectme\@there.com" ) ;
	System::submitSmallMessage( $spool_dir , "rejectme\@there.com" ) ;
	System::submitSmallMessage( $spool_dir , "rejectme1\@there.com" , "rejectme2\@there.com" ) ;
	Check::fileMatchCount( $spool_dir ."/emailrelay.*.envelope", 5 ) ;
	my $emailrelay = new Server( undef , undef , undef , $spool_dir ) ;
	$emailrelay->set_dst( $test_server->port() ) ;

	# test that the three messages with "rejectme" are left as ".bad"
	Check::ok( $emailrelay->run(\%args) , "failed to run" , $emailrelay->message() ) ;
	System::waitForFiles( $spool_dir ."/emailrelay.*.envelope.bad" , 3 ) ;
	System::waitForFiles( $spool_dir ."/emailrelay.*.content", 3 ) ;
	Check::fileMatchCount( $spool_dir ."/emailrelay.*.envelope.bad", 3 ) ;
	Check::fileMatchCount( $spool_dir ."/emailrelay.*.content", 3 ) ;
	Check::fileMatchCount( $spool_dir ."/emailrelay.*.envelope", 0 ) ;
	Check::allFilesContain( $spool_dir ."/emailrelay.*.envelope.bad" , "one or more recipients rejected" ) ;

	# tear down
	$test_server->kill() ;
	$emailrelay->wait() ;
	$test_server->cleanup() ;
	$emailrelay->cleanup() ;
}

sub testClientInvalidRecipientsWithForwardToSome
{
	# setup
	my %args = (
		Log => 1 ,
		Verbose => 1 ,
		Domain => 1 ,
		SpoolDir => 1 ,
		Forward => 1 ,
		ForwardTo => 1 ,
		ForwardToSome => 1 ,
		DontServe => 1 ,
		NoDaemon => 1 ,
		Hidden => 1 ,
	) ;
	my $spool_dir = System::createSpoolDir() ;
	my $test_server = new TestServer( System::nextPort() ) ;
	$test_server->run() ;
	System::submitSmallMessage( $spool_dir , "acceptme\@there.com" ) ;
	System::submitSmallMessage( $spool_dir , "acceptme1\@there.com" , "acceptme2\@there.com" ) ;
	System::submitSmallMessage( $spool_dir , "acceptme\@there.com" , "rejectme\@there.com" ) ;
	System::submitSmallMessage( $spool_dir , "rejectme\@there.com" ) ;
	System::submitSmallMessage( $spool_dir , "rejectme1\@there.com" , "rejectme2\@there.com" ) ;
	Check::fileMatchCount( $spool_dir ."/emailrelay.*.envelope", 5 ) ;
	my $emailrelay = new Server( undef , undef , undef , $spool_dir ) ;
	$emailrelay->set_dst( $test_server->port() ) ;

	# test that the three messages with "rejectme" are left as ".bad" and have no "acceptme" recipients
	Check::ok( $emailrelay->run(\%args) , "failed to run" , $emailrelay->message() ) ;
	System::waitForFiles( $spool_dir ."/emailrelay.*.envelope.bad" , 3 ) ;
	System::waitForFiles( $spool_dir ."/emailrelay.*.content", 3 ) ;
	Check::fileMatchCount( $spool_dir ."/emailrelay.*.envelope.bad", 3 ) ;
	Check::fileMatchCount( $spool_dir ."/emailrelay.*.content", 3 ) ;
	Check::fileMatchCount( $spool_dir ."/emailrelay.*.envelope", 0 ) ;
	Check::allFilesContain( $spool_dir ."/emailrelay.*.envelope.bad" , "recipients rejected" ) ;
	Check::noFileContains( $spool_dir ."/emailrelay.*.envelope.bad" , "acceptme" ) ;

	# tear down
	$test_server->kill() ;
	$emailrelay->wait() ;
	$test_server->cleanup() ;
	$emailrelay->cleanup() ;
}

sub _newOpenssl
{
	die "no test certificates: please use -C" if ! -f "$opt_certs_dir/alice.key" ;
	my $fs = new OpensslFileStore( $opt_certs_dir , ".pem" ) ;
	return new Openssl( $fs ) ;
}

sub _testTlsServer
{
	# setup
	my ( $client_cert_names , $client_ca_names , $server_cert_names , $server_ca_names , $server_verify , $expect_failure ) = @_ ;
	requireTls() ;
	requireOpensslTool() ; # Openssl::runClient()
	my %args = (
		Log => 1 ,
		Verbose => 1 ,
		Domain => 1 ,
		Port => 1 ,
		PidFile => 1 ,
		SpoolDir => 1 ,
		ServerTls => 1 ,
		ServerTlsRequired => 1 ,
		ServerTlsCertificate => 1 ,
		ServerTlsVerify => $server_verify ,
		TlsConfig => 1 ,
		Admin => 1 ,
		AdminTerminate => 1 ,
	) ;
	my $openssl = _newOpenssl() ;
	my $server_cert = $openssl->concatenate( @$server_cert_names ) ;
	my $server_ca = $openssl->concatenate( @$server_ca_names ) ;
	my $client_cert = $openssl->concatenate( @$client_cert_names ) ;
	my $client_ca = $openssl->concatenate( @$client_ca_names ) ;
	my $spool_dir = System::createSpoolDir() ;
	my $emailrelay = new Server( undef , undef , undef , $spool_dir , $server_cert , $server_ca ) ;
	my $admin_client = new AdminClient( $emailrelay->adminPort() , $System::localhost ) ;
	Check::ok( $emailrelay->run(\%args) , "failed to start" , $emailrelay->message() ) ;
	Check::ok( $admin_client->open() , "cannot connect for admin" , $emailrelay->adminPort() ) ;
	my $server_log = $emailrelay->stderr() ;

	# test
	my $client_log = System::tempfile( "sclient" ) ;
	OpensslRun::runClient( $System::localhost.":".$emailrelay->smtpPort() , $client_log , $client_cert , $client_ca ,
		sub { System::sleep_cs(200) ; $admin_client->doTerminate() } ) ;
	if( $expect_failure )
	{
		System::waitForFileLine( $server_log , "tls error" ) ;
		Check::fileDoesNotContain( $server_log , "tls.*established" ) ;
	}
	else
	{
		System::waitForFileLine( $server_log , "tls.*established" ) ;
		Check::fileContains( $client_log , "Session-ID" ) ;
	}

	# tear down
	$emailrelay->kill() ;
	$emailrelay->cleanup() ;
	$openssl->cleanup() ;
	System::unlink( $client_log ) ;
}

sub testTlsServerNoClientCertificateNoVerifyAccepted
{
	# c:trent <-- s:bob/dave
	my ( $verify , $expect_fail ) = ( 0 , 0 ) ;
	_testTlsServer(
		undef , ["trent.crt"] ,
		["bob.key","bob.crt","dave.crt"] , undef ,
		$verify , $expect_fail ) ;
}

sub testTlsServerNoClientCertificateVerifyRejected
{
	# c:trent <-- s:bob/dave
	# c:undef --> s:trent (v)
	my ( $verify , $expect_fail ) = ( 1 , 1 ) ;
	_testTlsServer(
		undef , ["trent.crt"] ,
		["bob.key","bob.crt","dave.crt"] , ["trent.crt"] ,
		$verify , $expect_fail ) ;
}

sub testTlsServerNoCaAccepted
{
	# c:trent <-- s:bob/dave
	# c:alice/carol --> s:undef (nv)
	my ( $verify , $expect_fail ) = ( 0 , 0 ) ;
	_testTlsServer(
		["alice.key","alice.crt","carol.crt"] , ["trent.crt"] ,
		["bob.key","bob.crt","dave.crt"] , undef ,
		$verify , $expect_fail ) ;
}

sub testTlsServerGoodClientCertificateNoVerifyAccepted
{
	# c:trent <-- s:bob/dave
	# c:alice/carol --> s:trent (nv)
	my ( $verify , $expect_fail ) = ( 0 , 0 ) ;
	_testTlsServer(
		["alice.key","alice.crt","carol.crt"] , ["trent.crt"] ,
		["bob.key","bob.crt","dave.crt"] , ["trent.crt"] ,
		$verify , $expect_fail ) ;
}

sub testTlsServerGoodClientCertificateVerifyAccepted
{
	# c:trent <-- s:bob/dave
	# c:alice/carol --> s:carol,trent (v)
	my ( $verify , $expect_fail ) = ( 1 , 0 ) ;
	_testTlsServer(
		["alice.key","alice.crt","carol.crt"] , ["trent.crt"] ,
		["bob.key","bob.crt","dave.crt"] , ["carol.crt","trent.crt"] , ### why is carol required in ca-list ?
		$verify , $expect_fail ) ;
}

sub testTlsServerBadClientCertificateNoVerifyAccepted
{
	# c:trent <-- s:bob/dave
	# c:malory --> s:trent (nv)
	my ( $verify , $expect_fail ) = ( 0 , 0 ) ;
	_testTlsServer(
		["malory.key","malory.crt"] , ["trent.crt"] ,
		["bob.key","bob.crt","dave.crt"] , ["trent.crt"] ,
		$verify , $expect_fail ) ;
}

sub testTlsServerBadClientCertificateVerifyRejected
{
	# c:trent <-- s:bob/dave
	# c:malory --> s:trent (v)
	my ( $verify , $expect_fail ) = ( 1 , 1 ) ;
	_testTlsServer(
		["malory.key","malory.crt"] , ["trent.crt"] ,
		["bob.key","bob.crt","dave.crt"] , ["trent.crt"] ,
		$verify , $expect_fail ) ;
}

sub _testTlsClient
{
	# setup
	my ( $client_cert_names , $client_ca_names , $server_cert_names , $server_ca_names , $client_verify , $expect_failure ) = @_ ;
	requireTls() ;
	requireOpensslTool() ; # Openssl::runServer()
	my %args = (
		Log => 1 ,
		Verbose => 1 ,
		Domain => 1 ,
		PidFile => 1 ,
		SpoolDir => 1 ,
		ClientTlsConnection => 1 ,
		ClientTlsCertificate => 1 ,
		ClientTlsVerify => $client_verify ,
		Admin => 1 ,
		AdminTerminate => 1 ,
		ForwardTo => 1 ,
		NoSmtp => 1 ,
		Poll => 1 ,
		TlsConfig => 1 ,
	) ;
	my $openssl = _newOpenssl() ;
	my $server_port = System::nextPort() ;
	my $client_cert = $openssl->concatenate( @$client_cert_names ) ;
	my $client_ca = $openssl->concatenate( @$client_ca_names ) ;
	my $server_cert = $openssl->concatenate( @$server_cert_names ) ;
	my $server_ca = $openssl->concatenate( @$server_ca_names ) ;
	my $spool_dir = System::createSpoolDir() ;
	System::submitSmallMessage( $spool_dir ) ;
	my $emailrelay = new Server( undef , undef , undef , $spool_dir , $client_cert , $client_ca ) ;
	$emailrelay->set_dst( $server_port ) ;
	my $admin_client = new AdminClient( $emailrelay->adminPort() , $System::localhost ) ;
	Check::ok( $emailrelay->run( \%args ) , "failed to start" , $emailrelay->message() ) ;
	Check::ok( $admin_client->open() , "cannot connect for admin" ) ;
	my $client_log = $emailrelay->stderr() ;

	# test
	my $server_log = System::tempfile( "sserver" ) ;
	OpensslRun::runServer( $server_port , $server_log , $server_cert , $server_ca ,
		sub { my ($pid) = @_ ; System::sleep_cs(200) ; $admin_client->doTerminate() ; System::killOnce($pid) } ,
		System::windows() ) ;
	if( $expect_failure )
	{
		Check::fileDoesNotContain( $client_log , "tls.*established" ) ;
		Check::fileContains( $client_log , "tls error" ) ;
		Check::fileContains( $server_log , "ERROR" ) ;
	}
	else
	{
		Check::fileContains( $client_log , "tls.*established" ) ;
		Check::fileDoesNotContain( $client_log , "tls error" ) ;
	}

	# tear down
	$openssl->cleanup() ;
	$emailrelay->kill() ;
	$emailrelay->cleanup() ;
	System::unlink( $server_log ) ;
}

sub testTlsClientGoodServerCertificateVerifyAccepted
{
	# c:trent (v) <-- s:bob/dave
	# c:alice/carol --> s:carol,trent
	my ( $verify , $expect_fail ) = ( 1 , 0 ) ;
	_testTlsClient(
		["alice.key","alice.crt","carol.crt"] , ["dave.crt","trent.crt"] , ### why is dave required in ca-list ?
		["bob.key","bob.crt","dave.crt"] , ["trent.crt"] ,
		$verify , $expect_fail ) ;
}

sub testTlsClientBadServerCertificateVerifyRejected
{
	# c:trent (v) <-- s:malory
	# c:alice/carol --> s:trent
	my ( $verify , $expect_fail ) = ( 1 , 1 ) ;
	_testTlsClient(
		["alice.key","alice.crt","carol.crt"] , ["trent.crt"] ,
		["malory.key","malory.crt"] , ["trent.crt"] ,
		$verify , $expect_fail ) ;
}

sub testTlsClientBadServerCertificateNoVerifyRejected
{
	# c:trent (nv) <-- s:malory
	# c:alice/carol --> s:trent
	my ( $verify , $expect_fail ) = ( 0 , 0 ) ;
	_testTlsClient(
		["alice.key","alice.crt","carol.crt"] , ["trent.crt"] ,
		["malory.key","malory.crt"] , ["trent.crt"] ,
		$verify , $expect_fail ) ;
}

sub _testTls
{
	# setup
	my ( $client_cert_names , $client_ca_names , $server_cert_names , $server_ca_names ,
		$client_verify , $server_verify , $expect_failure , $special ) = @_ ;
	$special ||= 0 ;
	requireTls() ;
	my %client_args = (
		Log => 1 ,
		Verbose => 1 ,
		Domain => 1 ,
		SpoolDir => 1 ,
		ClientTls => 1 ,
		ClientTlsCertificate => 1 ,
		ClientTlsVerify => $client_verify ,
		ForwardTo => 1 ,
		NoSmtp => 1 ,
		Poll => 2 ,
		PidFile => 1 ,
		TlsConfig => 1 ,
	) ;
	my %server_args = (
		Log => 1 ,
		Verbose => 1 ,
		Domain => 1 ,
		Port => 1 ,
		PidFile => 1 ,
		SpoolDir => 1 ,
		ServerTls => 1 ,
		ServerTlsRequired => 1 ,
		ServerTlsCertificate => 1 ,
		ServerTlsVerify => $server_verify ,
		TlsConfig => 1 ,
	) ;
	if( $special == 1 )
	{
		# no client tls and server doesnt require it
		delete $client_args{ClientTls} ;
		delete $client_args{ClientTlsCertificate} ;
		delete $client_args{ClientTlsVerify} ;
		delete $server_args{ServerTlsRequired} ;
	}
	if( $special == 2 )
	{
		# no client tls but server requires it
		delete $client_args{ClientTls} ;
		delete $client_args{ClientTlsCertificate} ;
		delete $client_args{ClientTlsVerify} ;
	}
	my $openssl = _newOpenssl() ;
	my $server_port = System::nextPort() ;
	my $client_cert = $openssl->concatenate( @$client_cert_names ) ;
	my $client_ca = $openssl->concatenate( @$client_ca_names ) ;
	my $server_cert = $openssl->concatenate( @$server_cert_names ) ;
	my $server_ca = $openssl->concatenate( @$server_ca_names ) ;
	my $client_spool_dir = System::createSpoolDir() ;
	my $server_spool_dir = System::createSpoolDir() ;
	System::submitSmallMessage( $client_spool_dir ) ;
	my $client = new Server( undef , undef , undef , $client_spool_dir , $client_cert , $client_ca ) ;
	my $server = new Server( $server_port , undef , undef , $server_spool_dir , $server_cert , $server_ca ) ;
	$client->set_dst( $server_port ) ;
	Check::ok( $server->run( \%server_args ) , "failed to start" , $server->message() ) ;
	$client->run( \%client_args ) ;
	my $client_log = $client->stderr() ;
	my $server_log = $server->stderr() ;

	# test
	if( $special == 1 )
	{
		System::waitForFiles( $server->spoolDir()."/emailrelay.*.envelope" , 1 ) ;
		System::waitForFileLine( $server_log , "envelope file:" ) ;
		Check::fileDoesNotContain( $server_log , "tls.*established" ) ;
		Check::fileDoesNotContain( $server_log , "tls error" ) ;
		Check::fileDoesNotContain( $client_log , "tls.*established" ) ;
		Check::fileDoesNotContain( $client_log , "tls error" ) ;
	}
	elsif( $special == 2 )
	{
		System::waitForFiles( $client->spoolDir()."/emailrelay.*.envelope.bad" , 1 ) ;
		System::waitForFileLine( $server_log , "encryption required" ) ;
		Check::fileDoesNotContain( $server_log , "tls.*established" ) ;
		Check::fileDoesNotContain( $server_log , "tls error" ) ;
		Check::fileDoesNotContain( $client_log , "tls.*established" ) ;
		Check::fileDoesNotContain( $client_log , "tls error" ) ;
	}
	elsif( $expect_failure )
	{
		System::waitForFiles( $client->spoolDir()."/emailrelay.*.envelope.bad" , 1 ) ;
		System::waitForFileLine( $client_log , "tls error" ) ;
		#Check::fileDoesNotContain( $client_log , "tls.*established" ) ; # not a valid check for v1.3
		System::waitForFileLine( $server_log , "tls error" ) ;
		Check::fileDoesNotContain( $server_log , "tls.*established" ) ;
	}
	else
	{
		System::waitForFiles( $server->spoolDir()."/emailrelay.*.envelope" , 1 ) ;
		System::waitForFileLine( $client_log , "tls.*established" ) ;
		Check::fileDoesNotContain( $client_log , "tls error" ) ;
		System::waitForFileLine( $server_log , "tls.*established" ) ;
		Check::fileDoesNotContain( $server_log , "tls error" ) ;
	}

	# tear down
	$server->kill() ;
	$client->kill() ;
	System::unlink( $client_log ) ;
	System::unlink( $server_log ) ;
	$openssl->cleanup() ;
	$client->cleanup() ;
	$server->cleanup() ;
}

sub testTlsGoodServerCertificateVerifyAccepted
{
	# c:trent (v) <-- s:bob/dave
	# c:alice/carol --> s:trent (v)
	my ( $client_verify , $server_verify , $expect_fail ) = ( 1 , 1 , 0 ) ;
	_testTls(
		["alice.key","alice.crt","carol.crt"] , ["trent.crt"] ,
		["bob.key","bob.crt","dave.crt"] , ["trent.crt"] ,
		$client_verify , $server_verify , $expect_fail ) ;
}

sub testTlsBadServerCertificateVerifyRejected
{
	# c:trent (v) <-- s:malory
	# c:alice/carol --> s:trent (nv)
	my ( $client_verify , $server_verify , $expect_fail ) = ( 1 , 0 , 1 ) ;
	_testTls(
		["alice.key","alice.crt","carol.crt"] , ["trent.crt"] ,
		["malory.key","malory.crt"] , ["trent.crt"] ,
		$client_verify , $server_verify , $expect_fail ) ;
}

sub testTlsBadServerCertificateNoVerifyAccepted
{
	# c:trent (nv) <-- s:malory
	# c:alice/carol --> s:trent (nv)
	my ( $client_verify , $server_verify , $expect_fail ) = ( 0 , 0 , 0 ) ;
	_testTls(
		["alice.key","alice.crt","carol.crt"] , ["trent.crt"] ,
		["malory.key","malory.crt"] , ["trent.crt"] ,
		$client_verify , $server_verify , $expect_fail ) ;
}

sub testTlsBadClientCertificateNoVerifyAccepted
{
	# c:trent (nv) <-- s:bob/dave
	# c:malory --> s:trent (nv)
	my ( $client_verify , $server_verify , $expect_fail ) = ( 0 , 0 , 0 ) ;
	_testTls(
		["malory.key","malory.crt"] , ["trent.crt"] ,
		["bob.key","bob.crt","dave.crt"] , ["trent.crt"] ,
		$client_verify , $server_verify , $expect_fail ) ;
}

sub testTlsBadClientCertificateVerifyRejected
{
	# c:trent (nv) <-- s:bob/dave
	# c:malory --> s:trent (v)
	my ( $client_verify , $server_verify , $expect_fail ) = ( 0 , 1 , 1 ) ;
	_testTls(
		["malory.key","malory.crt"] , ["trent.crt"] ,
		["bob.key","bob.crt","dave.crt"] , ["trent.crt"] ,
		$client_verify , $server_verify , $expect_fail ) ;
}

sub testTlsNoClientTlsAndServerDoesntCare
{
	my $special_test = 1 ;
	my ( $client_verify , $server_verify , $expect_fail ) = ( 1 , 1 , undef ) ;
	_testTls(
		["alice.key","alice.crt","carol.crt"] , ["trent.crt"] ,
		["bob.key","bob.crt","dave.crt"] , ["trent.crt"] ,
		$client_verify , $server_verify , $expect_fail , $special_test ) ;
}

sub testTlsNoClientTlsAndServerRequiresIt
{
	my $special_test = 2 ;
	my ( $client_verify , $server_verify , $expect_fail ) = ( 1 , 1 , undef ) ;
	_testTls(
		["alice.key","alice.crt","carol.crt"] , ["trent.crt"] ,
		["bob.key","bob.crt","dave.crt"] , ["trent.crt"] ,
		$client_verify , $server_verify , $expect_fail , $special_test ) ;
}

# ===

sub run
{
	my ( $name ) = @_ ;
	print $System::verbose ? "Running $name ...\n" : "running $name ... " ;
	eval $name."()";
	my $error = $@ ;
	if( $error )
	{
		chomp $error ;
		if( $error =~ m/^skip/ )
		{
			print $System::verbose ? "$name: $error\n\n" : "$error\n" ;
			return 77 ;
		}
		else
		{
			print $System::verbose ? "$name: failed: $error\n\n" : "failed: $error\n" ;
			$run_all_ok = 0 ;
			return 1 unless $opt_keep_going ;
		}
	}
	else
	{
		print $System::verbose ? "$name: passed\n\n" : "ok\n" ;
		return 0 ;
	}
}

# introspection
my @tests = () ;
my $f = new FileHandle( $0 ) ;
while( <$f> )
{
	my $line = $_ ; chomp $line ;
	if( $line =~ m/^sub test/ )
	{
		my @line_part = split( /\s+/ , $line ) ;
		push @tests , $line_part[1] ;
	}
}

# special command-line commands
if( scalar(@ARGV) == 1 && $ARGV[0] eq "list" )
{
	print join("\n",(@tests,"")) ;
	exit 0 ;
}
if( scalar(@ARGV) == 4 && $ARGV[0] eq "fixup" )
{
	my ( $fixup , $makefile_in , $makefile_out , $marker ) = @ARGV ;
	my $value = join(".test \\\n\t",@tests) . ".test" ;
	my $fh_in = new FileHandle( $makefile_in , "r" ) or die "error: cannot open [$makefile_in]\n" ;
	my $fh_out = new FileHandle( $makefile_out , "w" ) or die "error: cannot create [$makefile_out]\n" ;
	while(<$fh_in>)
	{
		chomp( my $line = $_ ) ;
		$line =~ s/$marker/$value/ ;
		print $fh_out $line , "\n" or die ;
	}
	$fh_out->close() or die ;
	exit 0 ;
}

# figure out which tests to run and which to skip
my %skip_tests = () ;
my %run_tests = () ;
my $run_tests = 0 ;
for my $arg ( @ARGV )
{
	$arg = File::Basename::basename($arg) =~ s/\.test$//r ;
	if( $arg =~ m/^-/ )
	{
		$skip_tests{substr($arg,1)} = 1 ;
	}
	else
	{
		$arg =~ s/^\+// ;
		$run_tests{$arg} = 1 ;
		$run_tests++ ;
	}
}

# run the tests
my $run_all_skipped = 1 ;
for my $test ( @tests )
{
	if( ( $run_tests && !exists $run_tests{$test} ) || $skip_tests{$test} )
	{
		print "(skipping $test)\n" unless $opt_quiet ;
	}
	else
	{
		my $result = run( $test ) ;
		$run_all_skipped = 0 unless $result == 77 ;
		last if $result == 1 ;
	}
}

# analyse the results and clean up
if( $run_all_skipped )
{
	exit 77 ;
}
elsif( $run_all_ok )
{
	killall() ;
	exit 0 ;
}
else
{
	killall() ;
	exit 1 ;
}
sub killall
{
	System::killAll( @Server::pid_list , @Helper::pid_list , @TestServer::pid_list ) ;
}
END
{
	killall() ;
}

