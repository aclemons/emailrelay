#!/usr/bin/env perl
#
# Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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
# Tests the E-MailRelay system.
#
# Some tests are skipped if not run as root. Timing parameters might
# need tweaking depending on the speed of the machine.
#
# usage: emailrelay-test.pl [-d <bin-dir>] [-x <test-bin-dir>] [-c <certs-dir>] [-k] [-v] [-t] [-T <config>] [<test-name> ...]
#      -d  - directory containing emailrelay binary
#      -x  - directory containing test program binaries
#      -c  - directory containing test certificates
#      -k  - keep going after a failed test
#      -v  - verbose logging from this script
#      -t  - keep temporary files
#      -T  - tls-config
#      -C  - create certs
#      -V  - use valgrind
#
# Use a dummy test name to get the list of tests available.
#
# Ubuntu package required: libnet-telnet-perl
#
# See also: man Net::Telnet
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
use Filter ;

$| = 1 ;

# parse the command line
my %opts = () ;
getopts( 'd:x:c:CkvtT:V' , \%opts ) or die ;
sub opt_bin_dir { return defined($opts{'d'}) ? $opts{'d'} : $_[0] }
sub opt_test_bin_dir { return defined($opts{'x'}) ? $opts{'x'} : $_[0] }
sub opt_certs_dir { return defined($opts{'c'}) ? $opts{'c'} : $_[0] }
sub opt_keep_going { return exists $opts{'k'} }
my $bin_dir = opt_bin_dir("../src/main") ;
my $test_bin_dir = opt_test_bin_dir(".") ;
my $certs_dir = opt_certs_dir("certificates") ;
$Server::bin_dir = $bin_dir ;
my $localhost = "127.0.0.1" ; # in case localhost resolves to ipv6 first
$Server::localhost = $localhost ;
$Server::tls_config = $opts{T} if exists $opts{T} ;
$Server::with_valgrind = $opts{V} if exists $opts{V} ;
$System::bin_dir = $bin_dir ;
$Helper::bin_dir = $test_bin_dir ;
$TestServer::bin_dir = $test_bin_dir ;
$System::verbose = 1 if exists $opts{'v'} ;
$System::keep = 1 if exists $opts{'t'} ;
$Openssl::keep = 1 if exists $opts{'t'} ;
createCerts() if exists($opts{C}) ;

my $run_all_ok = 1 ;

sub requireUnix
{
	if( ! System::unix() )
	{
		die "skipped: not unix\n" ;
	}
}

sub requireRoot
{
	# (typically called after requireUnix())
	my $id = `id -u` ;
	if( $id != 0 )
	{
		die "skipped: not root\n" ;
	}
}

sub requireDebug
{
	my $server = new Server() ;
	if( ! $server->hasDebug() )
	{
		die "skipped: not a debug build\n" ;
	}
}

sub requireThreads
{
	my $server = new Server() ;
	if( ! $server->hasThreads() )
	{
		die "skipped: not a multi-threaded build\n" ;
	}
}

sub requireTestAccount
{
	die "skipped: no non-root test account\n" if( getTestAccount() eq "root" ) ;
}

sub requireOpensslTool
{
	die "skipped: no openssl tool\n" if !Openssl::available() ;
}

sub getTestAccount
{
	if( System::mac() )
	{
		my $user = $ENV{LOGNAME} ;
		$user = $ENV{SUDO_USER} if( $user eq "root" ) ;
		return $user ;
	}
	else
	{
		return "bin" ;
	}
}

sub createCerts
{
	requireOpensslTool() ;
	mkdir $certs_dir if ! -d $certs_dir ;
	Openssl::createActorsIn( $certs_dir ) ;
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
	# setup
	my %args = (
		AsServer => 1 ,
		Domain => 1 ,
		Port => 1 ,
		PidFile => 1 ,
		SpoolDir => 1 ,
	) ;
	requireUnix() ;
	my $server = new Server() ;

	# test that the server starts up
	Check::ok( $server->run(\%args) , "failed to run" ) ;
	Check::that( $server->rc() == 0 , "immediate error" ) ;
	Check::fileExists( $server->stdout() , "stdout" ) ;
	Check::fileExists( $server->stderr() , "stderr" ) ;
	Check::fileExists( $server->pidFile() , "pid file" ) ;
	Check::numeric( $server->pid() , "pid file" ) ;
	Check::running( $server->pid() , $server->message() ) ;

	# test that the server stops on a signal
	my $c = new SmtpClient( $server->smtpPort() , $localhost ) ;
	Check::ok( $c->open(0) , "cannot connect for smtp" , $server->smtpPort() ) ;
	$server->kill() ;
	Check::notRunning( $server->pid() ) ;
	Check::fileDeleted( $server->pidFile() , "pid file" ) if System::unix() ;
	Check::fileEmpty( $server->stdout() , "stdout" ) ;
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
	Check::ok( $server->run(\%args) , "failed to run" ) ;
	Check::that( $server->rc() == 0 , "immediate error" ) ;
	Check::running( $server->pid() , $server->message() ) ;

	# test that the server can be terminated through the admin interface
	my $c = new AdminClient( $server->adminPort() , $localhost ) ;
	Check::ok( $c->open() , "cannot connect for admin" , $server->adminPort() ) ;
	$c->doHelp() ;
	$c->doHelp() ;
	$c->doTerminate() ;
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
	my $exe = System::mangledpath( System::exe( $bin_dir , "emailrelay-submit" ) ) ;
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
	my $exe = System::mangledpath( System::exe( $bin_dir , "emailrelay-passwd" ) ) ;
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
	my $exe = System::mangledpath( System::exe( $bin_dir , "emailrelay-passwd" ) ) ;
	my $fh = new FileHandle( "echo foobar | $exe --dotted |" ) ;
	my $result = <$fh> ;
	chomp $result ;
	my $dotted = "3507327710.2524437411.674156941.3275202688.3577739107.3692417735.1542828932.27631078" ;
	my $ok = ( $result eq $dotted ) ;
	Check::that( $ok , "password digest generation failed" ) ;
}

sub testSubmitPermissions
{
	# setup -- group-suid-daemon exe, group-daemon directory (requires world execute on all directories up to /)
	requireUnix() ;
	requireRoot() ;
	requireTestAccount() ;
	my $exe = System::tempfile("submit","/tmp") ;
	my $rc = system( "cp $bin_dir/emailrelay-submit $exe" ) ;
	$rc += system( "chown daemon:daemon $exe" ) ;
	$rc += system( "chmod 755 $exe" ) ;
	$rc += system( "chmod g+s $exe" ) ;
	Check::that( $rc == 0 , "cannot create suid submit exe" ) ;
	my $spool_dir = System::createSpoolDir() ;
	$rc = system( "chgrp daemon $spool_dir" ) ;
	$rc += system( "chmod 770 $spool_dir" ) ;
	Check::that( $rc == 0 , "cannot set spool dir permissions" ) ;
	my $path = System::createSmallMessageFile() ;
	$rc = system( "chmod 440 $path" ) ;
	Check::that( $rc == 0 , "cannot file permissions" ) ;

	# test that group-suid-daemon submit executable creates files correctly
	my $cmd = "$exe --from me\@here.localnet --spool-dir $spool_dir me\@there.localnet" ;
	my $someuser = getTestAccount() ;
	$rc = system( "cat $path | su -m $someuser -c \"$cmd\"" ) ;
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
	System::unlink( $path ) ;
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
	requireRoot() ;
	my $server = new Server() ;
	$server->run( \%args , "sudo " ) ;
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
	requireRoot() ;
	my $server = new Server() ;
	my $exe = System::tempfile("emailrelay") ;
	my $rc = system( "cp ".$server->exe()." $exe" ) ;
	$rc += system( "chmod 755 $exe" ) ;
	$rc += system( "sudo chown root $exe" ) ;
	$rc += system( "sudo chmod u+s $exe" ) ;
	Check::that( $rc == 0 ) ;
	$server->set_exe( $exe ) ;
	my $someuser = getTestAccount() ;
	$server->run( \%args , "sudo -u $someuser " ) ;
	Check::running( $server->pid() , $server->message() ) ;

	# test that the effective id is the real id ($someuser) and not root or --user
	Check::processRealUser( $server->pid() , $someuser ) ;
	Check::processEffectiveUser( $server->pid() , $someuser ) ;
	Check::processSavedUser( $server->pid() , "root" ) ;

	# tear down
	$server->kill() ;
	$server->cleanup() ;
	System::unlink( $exe ) ;
}

sub testServerSmtpSubmit
{
	# setup
	my %args = (
		AsServer => 1 ,
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
	my $client = new SmtpClient( $server->smtpPort() , $localhost ) ;
	$client->open() ;
	$client->submit_start() ;
	my $line = "lkjldkjfglkjdfglkjdferoiwuoiruwoeiur" ;
	for( my $i = 0 ; $i < 100 ; $i++ ) { $client->submit_line($line) }
	$client->submit_end() ;

	# test that message files appear in the spool directory
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
	System::deleteSpoolDir( $server->spoolDir() ) ;
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
	requireRoot() ;
	my $server = new Server() ;
	# for bsd/mac the file group is inherited from the directory - also assume the
	# group name is the same as the user name
	my $rc = 0 ;
	$rc = system( "chgrp ".$server->user()." ".$server->spoolDir() ) if ( System::bsd() || System::mac() ) ;
	$rc += system( "chmod 770 ".$server->spoolDir() ) if ( System::bsd() || System::mac() ) ;
	Check::that( $rc == 0 , "chgrp/chmod of spool directory failed" ) ;
	$server->run( \%args , "sudo " ) ;
	Check::running( $server->pid() , $server->message() ) ;
	my $client = new SmtpClient( $server->smtpPort() , $localhost ) ;
	$client->open() ;
	$client->submit() ;

	# test permissions of created files
	Check::fileOwner( $server->pidFile() , "root" ) ;
	Check::fileGroup( $server->pidFile() , $server->user() ) unless ( System::bsd() || System::mac() ) ;
	Check::fileOwner( System::match($server->spoolDir()."/emailrelay.*.content") , "root" ) ;
	Check::fileGroup( System::match($server->spoolDir()."/emailrelay.*.content") , $server->user() ) ;
	Check::fileMode( System::match($server->spoolDir()."/emailrelay.*.content") , 0660 ) ;
	Check::fileOwner( System::match($server->spoolDir()."/emailrelay.*.envelope") , "root" ) ;
	Check::fileGroup( System::match($server->spoolDir()."/emailrelay.*.envelope") , $server->user() ) ;
	Check::fileMode( System::match($server->spoolDir()."/emailrelay.*.envelope") , 0660 ) ;

	# tear down
	$server->kill() ;
	$server->cleanup() ;
	System::deleteSpoolDir( $server->spoolDir() ) ;
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
	my $pop = new PopClient( $server->popPort() , $localhost ) ;
	System::submitSmallMessage($server->spoolDir()) ;
	System::chmod_r( $server->spoolDir() , "700" , "600" ) ;

	# test that the pop client can log in and get a message list
	Check::ok( $pop->open() , "cannot connect to pop port" ) ;
	Check::ok( $pop->login( "me" , "secret" ) ) ;
	my @list = $pop->list(1,10) ;
	Check::that( scalar(@list) == 1 , "invalid message list" ) ;

	# tear down
	$server->kill() ;
	$server->cleanup() ;
	System::deleteSpoolDir( $server->spoolDir() ) ;
}

sub disabled_testServerPopList
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
	requireDebug() ;
	my $bufferbloat = !System::linux() ; # cannot check flow control on bufferbloated systems

	my $server = new Server() ;
	System::createFile( $server->popSecrets() , "server login me secret" ) ;
	$server->run( \%args , undef , "log-flow-control_large-pop-list" ) ; # G_TEST
	Check::running( $server->pid() , $server->message() ) ;
	my $pop = new PopClient( $server->popPort() , $localhost ) ;
	my $message_count = 40 ; # enough to trigger flow control? - tweak this
	for( my $i = 0 ; $i < $message_count ; $i++ ) { System::submitSmallMessage($server->spoolDir()) }
	System::chmod_r( $server->spoolDir() , "700" , "600" ) ;

	# test that the pop client can log in and get a message list and that flow control was exercised
	Check::ok( $pop->open() , "cannot connect to pop port" ) ;
	Check::ok( $pop->login( "me" , "secret" ) ) ;
	my @list = $pop->list(1,10) ;
	# (must be a debug build for this to work)
	Check::that( scalar(@list) == ($message_count * 1001) , "invalid message list (".scalar(@list).")" ) ;
	Check::fileContains( $server->stderr() , "flow control asserted" , "edit the test to add more messages" )
		unless $bufferbloat ;

	# tear down
	$server->kill() ;
	$server->cleanup() ;
	System::deleteSpoolDir( $server->spoolDir() ) ;
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
	my $pop = new PopClient( $server->popPort() , $localhost ) ;
	$pop->open() ;
	$pop->login( "me" , "secret" ) ;

	# test that the server sees the client disconnect
	System::waitForFileLine( $server->stderr() , "pop connection from" ) ;
	$pop->disconnect() ;
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
	Check::ok( $server->run(\%args) ) ;
	Check::running( $server->pid() , $server->message() ) ;

	# test for an appropriate protocol response if nothing to send
	my $c = new AdminClient( $server->adminPort() , $localhost ) ;
	Check::ok( $c->open() ) ;
	$c->doFlush() ;
	my $line = $c->getline() ;
	chomp $line ;
	Check::that( $line eq "error: no messages to send" , "unexpected response" , $line ) ;

	# tear down
	$server->kill() ;
	$server->cleanup() ;
	System::deleteSpoolDir( $spool_dir ) ;
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
	my $c = new AdminClient( $server->adminPort() , $localhost ) ;
	Check::ok( $c->open() ) ;
	$c->doFlush() ;
	my $line = $c->getline() ; $line ||= "" ;
	chomp $line ;
	Check::match( $line , "^error: dns error: no such host" , "unexpected response" ) ;
	Check::fileMatchCount( $spool_dir ."/emailrelay.*.envelope", 1 ) ;

	# tear down
	$server->kill() ;
	$server->cleanup() ;
	System::deleteSpoolDir($spool_dir) ;
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
	my $spool_dir_1 = System::createSpoolDir(undef,undef,"spool-1") ;
	my $spool_dir_2 = System::createSpoolDir(undef,undef,"spool-2") ;
	my $server_1 = new Server(undef,undef,undef,$spool_dir_1) ;
	my $server_2 = new Server($server_1->smtpPort()+100,undef,$server_1->adminPort()+100,$spool_dir_2) ;
	$server_1->set_dst("$localhost:".$server_2->smtpPort()) ;
	System::submitMessage( $spool_dir_1 , undef , 10000 ) ;
	System::submitMessage( $spool_dir_1 , undef , 10000 ) ;
	Check::ok( $server_1->run(\%args) ) ;
	Check::ok( $server_2->run(\%args) ) ;
	Check::running( $server_1->pid() , $server_1->message() ) ;
	Check::running( $server_2->pid() , $server_2->message() ) ;
	my $c = new AdminClient( $server_1->adminPort() , $localhost ) ;
	Check::ok( $c->open() ) ;
	Check::fileMatchCount( $spool_dir_1 ."/emailrelay.*.content", 2 ) ;
	Check::fileMatchCount( $spool_dir_1 ."/emailrelay.*.envelope", 2 ) ;
	Check::fileMatchCount( $spool_dir_2 ."/emailrelay.*.content", 0 ) ;
	Check::fileMatchCount( $spool_dir_2 ."/emailrelay.*.envelope", 0 ) ;

	# test that messages are sent from an admin flush command
	$c->doFlush() ;
	my $line = $c->getline( 30 ) ;
	chomp $line ;
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
	my $spool_dir_1 = System::createSpoolDir(undef,undef,"spool-1") ;
	my $spool_dir_2 = System::createSpoolDir(undef,undef,"spool-2") ;
	my $server_1 = new Server(undef,undef,undef,$spool_dir_1) ;
	my $server_2 = new Server($server_1->smtpPort()+100,undef,$server_1->adminPort()+100,$spool_dir_2) ;
	$server_1->set_dst("$localhost:".$server_2->smtpPort()) ;
	System::submitMessage( $spool_dir_1 , undef , 10000 ) ;
	$server_1->run(\%args) ;
	$server_2->run(\%args) ;
	Check::running( $server_1->pid() , $server_1->message() ) ;
	Check::running( $server_2->pid() , $server_2->message() ) ;
	my $c = new AdminClient( $server_1->adminPort() , $localhost ) ;
	Check::ok( $c->open() ) ;

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
	Check::ok( $server->run(\%args) ) ;
	Check::running( $server->pid() , $server->message() ) ;

	# test that the server drops the connection if we mess up the client protocol
	my $c = new SmtpClient( $server->smtpPort() , $localhost ) ;
	Check::ok( $c->open() ) ;
	$c->doBadHelo() ;
	my $seen_reset = 0 ;
	for( my $i = 0 ; $i < 10 ; $i++ )
	{
		my $error = $c->doBadCommand() ;
		my $was_reset =
			( $error =~ m/Connection reset by peer/ ) ||
			( $error =~ m/pattern match read eof/ ) ||
			( $error =~ m/filehandle isn.t open/ ) ;
		$seen_reset = $seen_reset || $was_reset ;
		System::sleep_cs( 20 ) ;
	}
	Check::that( $seen_reset , "connection not dropped" ) ;

	# tear down
	$server->kill() ;
	$server->cleanup() ;
	System::deleteSpoolDir( $server->spoolDir() ) ;
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
	Check::ok( $server->run(\%args) ) ;
	Check::running( $server->pid() , $server->message() ) ;
	my $c = new SmtpClient( $server->smtpPort() , $localhost ) ;
	Check::ok( $c->open() ) ;
	my $line = "0123456789 123456789 123456789 123456789 123456789" ;
	$line .= $line ;

	# test that if the server aborts the connection if the client submits a big message
	$c->submit_start() ;
	for( my $i = 0 ; $i < 12 ; $i++ )
	{
		$c->submit_line( $line ) ;
	}
	eval { $c->submit_end() } ;
	my $error = $@ ;
	Check::that( $error ne "" , "large message submission did not fail as it should have" ) ;
	Check::fileMatchCount( $server->spoolDir()."/emailrelay.*.content" , 0 ) ;
	Check::fileMatchCount( $server->spoolDir()."/emailrelay.*.envelope*" , 0 ) ;
	Check::fileContains( $server->stderr() , "552 message exceeds fixed maximum message size" ) ;

	# tear down
	$server->kill() ;
	$server->cleanup() ;
	System::deleteSpoolDir( $server->spoolDir() ) ;
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
	$client->set_dst( "$localhost:".$server->smtpPort() ) ;
	System::createFile( $server->serverSecrets() , "server login me secret" ) ;
	System::submitSmallMessage( $client->spoolDir() ) ;
	Check::ok( $server->run(\%server_args) , "failed to start server" ) ;
	System::waitForFile( $server->stderr() ) ;

	# test that the client gets as far as issuing the mail command
	my $ok = $client->run( \%client_args ) ;
	Check::ok( $ok ) ;
	System::waitForFileLine( $server->stderr() , "MAIL FROM:" ) ;
	System::waitForFileLine( $server->stderr() , "530 authentication required" ) ;

	# tear down
	$server->kill() ;
	$server->cleanup() ;
	$client->cleanup() ;
	System::deleteSpoolDir( $server->spoolDir() ) ;
	System::deleteSpoolDir( $client->spoolDir() , 1 ) ;
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
	$client->set_dst( "$localhost:".$test_server->port() ) ;
	$test_server->run( "--fail-at 1" ) ;
	System::submitSmallMessage( $client->spoolDir() ) ;
	System::submitSmallMessage( $client->spoolDir() ) ;

	# test that the client runs but the failed message envelope has a reason code
	Check::ok( $client->run(\%args) , "failed to run emailrelay as client" ) ;
	System::waitForFiles( $client->spoolDir()."/emailrelay.*.envelope*bad" , 1 , "envelope" ) ;
	Check::fileMatchCount( $client->spoolDir()."/emailrelay.*.content" , 1 , "content" ) ;
	my @files = System::glob_( $client->spoolDir()."/emailrelay*bad" ) ;
	Check::fileContains( $files[0] , "X-MailRelay-ReasonCode: 452" ) ;

	# tear down
	$client->kill() ;
	$client->cleanup() ;
	$test_server->kill() ;
	$test_server->cleanup() ;
	System::deleteSpoolDir( $client->spoolDir() , 1 ) ;
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
	my $outputfile = System::tempfile("output",System::unix()?"/tmp":undef) ; # /tmp is writeable by daemon
	Filter::create( $server->filter() , {
			unix => [
				"echo \"\$\@\" | sed 's/^/arg: /' > $outputfile" ,
				"env | sed 's/^/env: /' >> $outputfile" ,
				"exit 0" ,
			] ,
			win32 => [
				"var fs = WScript.CreateObject(\"Scripting.FileSystemObject\");" ,
				"var out_ = fs.OpenTextFile( \"$outputfile\" , 8 , true ) ;" ,
				"out_.WriteLine( \"arg: \" + WScript.Arguments(0) ) ;" ,
				"WScript.Quit(0);" ,
			] ,
		} ) ;
	Check::ok( $server->run(\%args) ) ;
	Check::running( $server->pid() , $server->message() ) ;
	my $c = new SmtpClient( $server->smtpPort() , $localhost ) ;
	Check::ok( $c->open() ) ;

	# test that the filter is executed with the correct environment
	$c->submit() ;
	Check::that( -f $outputfile , "filter did not create an output file" ) ;
	Check::fileContains( $outputfile , "arg: " , "filter did not generate output" ) ;
	Check::fileContains( $outputfile , "arg: /" , "filter not passed an absolute path" ) if System::unix() ;
	Check::fileLineCountLessThan( $outputfile , 7 , "env: " , "wrong number of environment variables" ) if System::unix() ;
	Check::fileMatchCount( $server->spoolDir()."/emailrelay.*.content" , 1 ) ;
	Check::fileMatchCount( $server->spoolDir()."/emailrelay.*.envelope" , 1 ) ;

	# tear down
	System::unlink( $outputfile ) ;
	$server->kill() ;
	$server->cleanup() ;
	System::deleteSpoolDir( $server->spoolDir() ) ;
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
	my $outputfile = System::tempfile("output","/tmp") ;
	Filter::create( $server->filter() , {
			unix => [
				'ps -p $$'." -o uid,ruid | sed 's/  */ /g' | sed 's/^ *//' > $outputfile" ,
				"exit 0" ,
			] ,
			win32 => [
				# not used
			] ,
		} ) ;
	Check::ok( $server->run(\%args) ) ;
	Check::running( $server->pid() , $server->message() ) ;
	my $c = new SmtpClient( $server->smtpPort() , $localhost ) ;
	Check::ok( $c->open() ) ;

	# test that the filter is executed with the correct identity
	$c->submit() ;
	Check::that( -f $outputfile , "filter did not create an output file" ) ;
	my $uid1 = System::uid( $server->user() ) ;
	my $uid2 = $uid1 - 4294967296 ; # negative ids on mac
	Check::fileContainsEither( $outputfile , "$uid1 $uid1" , "$uid2 $uid2" , "filter not run as uid $uid1" ) ;

	# tear down
	System::unlink( $outputfile ) ;
	$server->kill() ;
	$server->cleanup() ;
	System::deleteSpoolDir( $server->spoolDir() ) ;
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
	Filter::create( $server->filter() , {
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
	Check::ok( $server->run(\%args) ) ;
	Check::running( $server->pid() , $server->message() ) ;
	my $c = new SmtpClient( $server->smtpPort() , $localhost ) ;
	Check::ok( $c->open() ) ;

	# test that if the filter rejects the message then the submit fails and no files are spooled
	$c->submit( 1 ) ;
	Check::fileMatchCount( $server->spoolDir()."/emailrelay.*.content" , 0 ) ;
	Check::fileMatchCount( $server->spoolDir()."/emailrelay.*.envelope*" , 0 ) ;
	Check::fileContains( $server->stderr() , "rejected by filter: .foo bar." ) ;

	# tear down
	$server->kill() ;
	$server->cleanup() ;
	System::deleteSpoolDir( $server->spoolDir() ) ;
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
		Filter => 1 ,
		ForwardTo => 1 ,
		Immediate => 1 ,
	) ;
	my $server_1 = new Server() ;
	my $server_2 = new Server($server_1->smtpPort()+100) ;
	$server_1->set_dst("$localhost:".$server_2->smtpPort()) ;
	Filter::create( $server_1->filter() , {
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
	Check::ok( $server_1->run(\%args) ) ;
	delete $args{Filter} ;
	delete $args{ForwardTo} ;
	delete $args{Immediate} ;
	Check::ok( $server_2->run(\%args) , $server_2->rc() ) ;
	Check::running( $server_1->pid() , $server_1->message() ) ;
	Check::running( $server_2->pid() , $server_2->message() ) ;
	my $c = new SmtpClient( $server_1->smtpPort() , $localhost ) ;
	Check::ok( $c->open() ) ;

	# test that if the filter deletes the message files then proxying succeeds or fails depending on the exit code
	$c->submit($expect_submit_error) ;

	# tear down
	$server_1->kill() ;
	$server_2->kill() ;
	$server_1->cleanup() ;
	$server_2->cleanup() ;
	System::deleteSpoolDir( $server_1->spoolDir() ) ;
	System::deleteSpoolDir( $server_2->spoolDir() ) ;
}

sub testFilterPollTimeout
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
	Filter::create( $server->filter() , {
			unix => [
				"exit 103" ,
			] ,
			win32 => [
				"WScript.Quit(103) ;" ,
			] ,
		} ) ;
	Check::ok( $server->run(\%args) , $server->message() ) ;
	Check::running( $server->pid() , $server->message() ) ;
	my $c = new SmtpClient( $server->smtpPort() , $localhost ) ;
	Check::ok( $c->open() ) ;

	# test that the rescan is triggered
	$c->submit() ;
	System::waitForFileLine( $server->stderr() , "forwarding: .rescan" , "no rescan message in the log file" ) ;

	# tear down
	$server->kill() ;
	$server->cleanup() ;
	System::deleteSpoolDir( $server->spoolDir() ) ;
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
	Filter::create( $server->filter() , {
			unix => [
				"sleep 3" ,
				"exit 0" ,
			] ,
			win32 => [
				"WScript.Sleep(3000) ;" ,
				"WScript.Quit(0) ;" ,
			] ,
		} ) ;
	Check::ok( $server->run(\%args) , $server->message() ) ;
	Check::running( $server->pid() , $server->message() ) ;
	my $c1 = new SmtpClient( $server->smtpPort() , $localhost ) ;
	my $c2 = new SmtpClient( $server->smtpPort() , $localhost ) ;
	Check::ok( $c1->open() ) ;
	Check::ok( $c2->open() ) ;
	$c1->submit( undef , 1 ) ; # 1=>no-wait
	$c2->submit( undef , 1 ) ;

	# test that the two messages commit at roughly the same time
	sleep( 1 ) ;
	Check::fileMatchCount( $server->spoolDir()."/emailrelay.*.envelope" , 0 ) ;
	sleep( 3 ) ;
	Check::fileMatchCount( $server->spoolDir()."/emailrelay.*.envelope" , 2 ) ;

	# tear down
	$server->kill() ;
	$server->cleanup() ;
	System::deleteSpoolDir( $server->spoolDir() ) ;
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
	my $scanner = new Scanner( $server->scannerPort() ) ;
	Check::ok( $server->run(\%args) , $server->message() ) ;
	Check::running( $server->pid() , $server->message() ) ;
	$scanner->run() ;
	my $c = new SmtpClient( $server->smtpPort() , $localhost ) ;
	Check::ok( $c->open() ) ;

	# test that the scanner is used
	$c->submit_start() ;
	$c->submit_line( "send ok" ) ; # (the test scanner treats the message body as a script)
	$c->submit_end() ;
	Check::fileContains( $scanner->logfile() , "new connection from" ) ;
	Check::fileContains( $scanner->logfile() , "send ok" ) ;
	Check::fileDoesNotContain( $server->stderr() , "452 " ) ;
	Check::fileMatchCount( $server->spoolDir()."/emailrelay.*.envelope" , 1 ) ;

	# tear down
	$server->kill() ;
	$scanner->kill() ;
	$server->cleanup() ;
	$scanner->cleanup() ;
	System::deleteSpoolDir( $server->spoolDir() ) ;
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
Debug => 1 ,
	) ;
	my $server = new Server() ;
	my $scanner = new Scanner( $server->scannerPort() ) ;
	Check::ok( $server->run(\%args) , $server->message() ) ;
	Check::running( $server->pid() , $server->message() ) ;
	$scanner->run() ;
	my $c = new SmtpClient( $server->smtpPort() , $localhost ) ;
	Check::ok( $c->open() ) ;

	# test that the scanner is used
	$c->submit_start() ;
	$c->submit_line( "send foobar" ) ; # (the test scanner treats the message body as a script)
	$c->submit_end( 1 ) ; # 1 <= expect the "." command to fail
	Check::fileContains( $server->stderr() , "rejected by filter: .foobar" ) ;
	Check::fileContains( $server->stderr() , "452 foobar" ) ;
	Check::fileMatchCount( $server->spoolDir()."/emailrelay.*.envelope" , 0 ) ;

	# tear down
	$server->kill() ;
	$scanner->kill() ;
	$server->cleanup() ;
	$scanner->cleanup() ;
	System::deleteSpoolDir( $server->spoolDir() ) ;
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
	my $scanner = new Scanner( $server->scannerPort() ) ;
	Check::ok( $server->run(\%args) , $server->message() ) ;
	Check::running( $server->pid() , $server->message() ) ;
	$scanner->run() ;
	my $c = new SmtpClient( $server->smtpPort() , $localhost ) ;
	Check::ok( $c->open() ) ;

	# test that the scanner is used
	$c->submit_start() ;
	$c->submit_line( "sleep 3" ) ;
	$c->submit_line( "send foobar" ) ;
	$c->submit_end( 1 ) ;
	Check::fileDoesNotContain( $server->stderr() , "452 foobar" ) ;
	Check::fileContains( $server->stderr() , "452 .*time.*out" ) ;

	# tear down
	$server->kill() ;
	$scanner->kill() ;
	$server->cleanup() ;
	$scanner->cleanup() ;
	System::deleteSpoolDir( $server->spoolDir() ) ;
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
	Check::ok( $server->run(\%args) , $server->message() ) ;
	Check::running( $server->pid() , $server->message() ) ;
	$verifier->run() ;
	my $c = new SmtpClient( $server->smtpPort() , $localhost ) ;
	Check::ok( $c->open() ) ;

	# test that the verifier is used
	$c->submit_start( "OK\@here" ) ; # the test verifier interprets this string
	$c->submit_line( "just testing" ) ;
	$c->submit_end() ;
	Check::fileContains( $verifier->logfile() , "sending valid" ) ;
	Check::fileDoesNotContain( $server->stderr() , "452 " ) ;
	Check::fileMatchCount( $server->spoolDir()."/emailrelay.*.envelope" , 1 ) ;

	# tear down
	$server->kill() ;
	$verifier->kill() ;
	$server->cleanup() ;
	$verifier->cleanup() ;
	System::deleteSpoolDir( $server->spoolDir() ) ;
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
	Check::ok( $server->run(\%args) , $server->message() ) ;
	Check::running( $server->pid() , $server->message() ) ;
	$verifier->run() ;
	my $c = new SmtpClient( $server->smtpPort() , $localhost ) ;
	Check::ok( $c->open() ) ;

	# test that the verifier can reject
	$c->submit_start( "fail\@here" , 1 ) ; # the test verifier interprets this string
	Check::fileContains( $verifier->logfile() , "sending error" ) ;
	Check::fileContains( $server->stderr() , "VerifierError" ) ; # see emailrelay_test_verifier.cpp
	Check::fileMatchCount( $server->spoolDir()."/emailrelay.*.envelope" , 0 ) ;

	# tear down
	$server->kill() ;
	$verifier->kill() ;
	$server->cleanup() ;
	$verifier->cleanup() ;
	System::deleteSpoolDir( $server->spoolDir() ) ;
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
	my $spool_dir_1 = System::createSpoolDir(undef,undef,"spool-1") ;
	my $spool_dir_2 = System::createSpoolDir(undef,undef,"spool-2") ;
	my $server_1 = new Server(undef,undef,undef,$spool_dir_1) ;
	my $server_2 = new Server($server_1->smtpPort()+100,undef,$server_1->adminPort()+100,$spool_dir_2) ;
	$server_1->set_dst("$localhost:".$server_2->smtpPort()) ;
	Check::ok( $server_1->run(\%args) ) ;
	delete $args{Immediate} ;
	delete $args{ForwardTo} ;
	Check::ok( $server_2->run(\%args) ) ;
	Check::running( $server_1->pid() , $server_1->message() ) ;
	Check::running( $server_2->pid() , $server_2->message() ) ;
	my $c = new SmtpClient( $server_1->smtpPort() , $localhost ) ;
	Check::ok( $c->open() ) ;
	Check::fileMatchCount( $spool_dir_1 ."/emailrelay.*", 0 ) ;
	Check::fileMatchCount( $spool_dir_2 ."/emailrelay.*", 0 ) ;

	# test that the proxy uses one connection to forward multiple messages
	my $n = 4 ;
	for( my $i = 0 ; $i < $n ; $i++ )
	{
		$c->submit_start() ;
		$c->submit_line( "foo bar" ) ;
		$c->submit_end() ;
	}
	$c->close() ;
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
	my $spool_dir_1 = System::createSpoolDir(undef,undef,"spool-1") ;
	my $spool_dir_2 = System::createSpoolDir(undef,undef,"spool-2") ;
	System::submitSmallMessage($spool_dir_1) ;
	System::submitSmallMessage($spool_dir_1) ;
	Check::fileMatchCount( $spool_dir_1 ."/emailrelay.*.envelope", 2 ) ;
	my $server_1 = new Server(undef,undef,undef,$spool_dir_1) ;
	my $server_2 = new Server($server_1->smtpPort()+100,undef,$server_1->adminPort()+100,$spool_dir_2) ;
	my %server_args = %args ;
	delete $server_args{Forward} ;
	delete $server_args{ForwardTo} ;
	delete $server_args{ClientFilter} ;
	delete $server_args{DontServe} ;
	delete $server_args{NoDaemon} ;
	$server_args{PidFile} = 1 ;
	$server_args{Port} = 1 ;
	Check::ok( $server_2->run(\%server_args) ) ;
	Check::running( $server_2->pid() , $server_2->message() ) ;
	$server_1->set_dst("$localhost:".$server_2->smtpPort()) ;
	my $outputfile = System::tempfile("output",System::unix()?"/tmp":undef) ; # /tmp is writeable by daemon
	Filter::create( $server_1->clientFilter() , {
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
	Check::ok( $server_1->run(\%args) ) ;

	# test that the client filter runs and the messages are forwarded
	System::waitForFiles( $spool_dir_2 ."/emailrelay.*" , 4 ) ;
	System::waitForFiles( $spool_dir_1 . "/emailrelay.*" , 0 ) ;
	Check::fileExists( $outputfile , "no output file generated by the client filter" ) ;
	Check::fileMatchCount( $spool_dir_2 ."/emailrelay.*.envelope", 2 ) ;
	Check::fileMatchCount( $spool_dir_2 ."/emailrelay.*.content", 2 ) ;

	# tear down
	$server_1->kill() ;
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
	my $spool_dir_1 = System::createSpoolDir(undef,undef,"spool-1") ;
	my $spool_dir_2 = System::createSpoolDir(undef,undef,"spool-2") ;
	System::submitSmallMessage($spool_dir_1) ;
	System::submitSmallMessage($spool_dir_1) ;
	Check::fileMatchCount( $spool_dir_1 ."/emailrelay.*.envelope", 2 ) ;
	my $server_1 = new Server(undef,undef,undef,$spool_dir_1) ;
	my $server_2 = new Server($server_1->smtpPort()+100,undef,$server_1->adminPort()+100,$spool_dir_2) ;
	my %server_args = %args ;
	delete $server_args{Forward} ;
	delete $server_args{ForwardTo} ;
	delete $server_args{ClientFilter} ;
	delete $server_args{DontServe} ;
	delete $server_args{NoDaemon} ;
	delete $server_args{Hidden} ;
	$server_args{PidFile} = 1 ;
	$server_args{Port} = 1 ;
	Check::ok( $server_2->run(\%server_args) ) ;
	Check::running( $server_2->pid() , $server_2->message() ) ;
	$server_1->set_dst("$localhost:".$server_2->smtpPort()) ;
	my $outputfile = System::tempfile("output",System::unix()?"/tmp":undef) ; # /tmp is writeable by daemon
	Filter::create( $server_1->clientFilter() , {
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
	Check::ok( $server_1->run(\%args) ) ;

	# test that the client filter runs and the messages are failed
	System::waitForFiles( $spool_dir_1 ."/emailrelay.*.bad", 2 ) ;
	Check::fileMatchCount( $spool_dir_1 ."/emailrelay.*.content", 2 ) ;
	Check::fileExists( $outputfile , "no output file generated by the client filter" ) ;
	Check::fileMatchCount( $spool_dir_2 ."/emailrelay.*", 0 ) ;

	# tear down
	$server_1->kill() ;
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
	System::submitSmallMessage($spool_dir) ;
	System::submitSmallMessage($spool_dir) ;
	Check::fileMatchCount( $spool_dir ."/emailrelay.*.envelope", 2 ) ;
	my $emailrelay = new Server( undef , undef , undef , $spool_dir ) ;
	System::createFile( $emailrelay->clientSecrets() , "client login me secret" ) ;
	$emailrelay->set_dst( "$localhost:".$test_server->port() ) ;

	# test that protocol fails and one message fails
	Check::ok( $emailrelay->run( \%args ) ) ;
	System::waitForFileLine( $emailrelay->stderr() , "cannot do authentication required by " ) ;
	Check::fileMatchCount( $spool_dir ."/emailrelay.*.envelope", 1 ) ;
	Check::fileMatchCount( $spool_dir ."/emailrelay.*.envelope.bad", 1 ) ;

	# tear down
	$emailrelay->cleanup() ;
	$test_server->cleanup() ;
	System::deleteSpoolDir( $spool_dir , 1 ) ;
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
	System::submitSmallMessage($spool_dir) ;
	System::submitSmallMessage($spool_dir) ;
	Check::fileMatchCount( $spool_dir ."/emailrelay.*.envelope", 2 ) ;
	my $emailrelay = new Server( undef , undef , undef , $spool_dir ) ;
	System::createFile( $emailrelay->clientSecrets() , "client login me secret" ) ;
	$emailrelay->set_dst( "$localhost:".$test_server->port() ) ;

	# test that protocol fails and one message fails
	Check::ok( $emailrelay->run( \%args ) ) ;
	System::waitForFileLine( $emailrelay->stderr() , "authentication failed" ) ;
	System::waitForFiles( $spool_dir ."/emailrelay.*.envelope.bad", 1 ) ;
	Check::fileMatchCount( $spool_dir ."/emailrelay.*.envelope", 1 ) ;

	# tear down
	$emailrelay->cleanup() ;
	$test_server->kill() ;
	$test_server->cleanup() ;
	System::deleteSpoolDir( $spool_dir , 1 ) ;
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
	System::submitSmallMessage($spool_dir) ;
	System::submitSmallMessage($spool_dir) ;
	System::submitSmallMessage($spool_dir) ;
	System::submitSmallMessage($spool_dir) ;
	Check::fileMatchCount( $spool_dir ."/emailrelay.*.envelope", 4 ) ;
	my $emailrelay = new Server( undef , undef , undef , $spool_dir ) ;
	$emailrelay->set_dst( "$localhost:".$test_server->port() ) ;

	# test that two of the four messages are left as ".bad"
	Check::ok( $emailrelay->run( \%args ) ) ;
	System::waitForFiles( $spool_dir ."/emailrelay.*.bad" , 2 ) ;
	Check::fileMatchCount( $spool_dir ."/emailrelay.*.content", 2 ) ;
	Check::fileMatchCount( $spool_dir ."/emailrelay.*.envelope.bad", 2 ) ;
	Check::fileMatchCount( $spool_dir ."/emailrelay.*.envelope", 0 ) ;

	# tear down
	$emailrelay->cleanup() ;
	$test_server->cleanup() ;
	System::deleteSpoolDir( $spool_dir , 1 ) ;
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
	System::submitSmallMessage( $spool_dir , undef , "acceptme\@there.com" ) ;
	System::submitSmallMessage( $spool_dir , undef , "acceptme1\@there.com" , "acceptme2\@there.com" ) ;
	System::submitSmallMessage( $spool_dir , undef , "acceptme\@there.com" , "rejectme\@there.com" ) ;
	System::submitSmallMessage( $spool_dir , undef , "rejectme\@there.com" ) ;
	System::submitSmallMessage( $spool_dir , undef , "rejectme1\@there.com" , "rejectme2\@there.com" ) ;
	Check::fileMatchCount( $spool_dir ."/emailrelay.*.envelope", 5 ) ;
	my $emailrelay = new Server( undef , undef , undef , $spool_dir ) ;
	$emailrelay->set_dst( "$localhost:".$test_server->port() ) ;

	# test that the three "rejectme" messages out of five are left as ".bad"
	Check::ok( $emailrelay->run( \%args ) ) ;
	System::waitForFiles( $spool_dir ."/emailrelay.*.envelope.bad" , 3 ) ;
	System::waitForFiles( $spool_dir ."/emailrelay.*.content", 3 ) ;
	Check::fileMatchCount( $spool_dir ."/emailrelay.*.envelope.bad", 3 ) ;
	Check::fileMatchCount( $spool_dir ."/emailrelay.*.content", 3 ) ;
	Check::fileMatchCount( $spool_dir ."/emailrelay.*.envelope", 0 ) ;
	Check::allFilesContain( $spool_dir ."/emailrelay.*.envelope.bad" , "one or more recipients rejected" ) ;

	# tear down
	$emailrelay->cleanup() ;
	$test_server->cleanup() ;
	System::deleteSpoolDir( $spool_dir , 1 ) ;
}

sub _newOpenssl
{
	my $openssl ;
	if( -f "$certs_dir/alice.key" ) # if pre-prepared by "-C"
	{
		unlink( System::glob_("$certs_dir/*-*") ) ;
		unlink( System::glob_("$certs_dir/*.out") ) ;
		$openssl = new Openssl( sub{"$certs_dir/".$_[0]} , sub{System::log_($_[0])} ) ;
		$openssl->readActors() ;
	}
	else
	{
		requireOpensslTool() ;
		$openssl = new Openssl( sub{System::tempfile($_[0])} , sub{System::log_($_[0])} ) ;
		$openssl->createActors() ;
	}
	return $openssl ;
}

sub _testTlsServer
{
	# setup
	my ( $client_cert_names , $client_ca_names , $server_cert_names , $server_ca_names , $server_verify , $expect_failure ) = @_ ;
	requireOpensslTool() ;
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
	my $server_cert = $server_cert_names ? $openssl->file( @$server_cert_names ) : undef ;
	my $server_ca = $server_ca_names ? $openssl->file( @$server_ca_names ) : undef ;
	my $client_cert = $client_cert_names ? $openssl->file( @$client_cert_names ) : undef ;
	my $client_ca = $client_ca_names ? $openssl->file( @$client_ca_names ) : undef ;
	my $spool_dir = System::createSpoolDir() ;
	my $emailrelay = new Server( undef , undef , undef , $spool_dir , undef , $server_cert , $server_ca ) ;
	my $admin_client = new AdminClient( $emailrelay->adminPort() , $localhost ) ;
	Check::ok( $emailrelay->run( \%args ) , "failed to start emailrelay server" ) ;
	Check::ok( $admin_client->open() , "cannot connect for admin" , $emailrelay->adminPort() ) ;
	my $server_log = $emailrelay->stderr() ;

	# test
	my $client_log = System::tempfile( "sclient" ) ;
	$openssl->runClient( "$localhost:".$emailrelay->smtpPort() , $client_log , $client_cert , $client_ca ,
		sub { Server::sleep_cs(100) ; $admin_client->doTerminate() } ) ;
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
	$emailrelay->cleanup() ;
	$openssl->cleanup() ;
	System::deleteSpoolDir( $spool_dir , 1 ) ;
	System::unlink( $server_log ) ;
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
	requireOpensslTool() ;
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
		Poll => 1
	) ;
	my $openssl = _newOpenssl() ;
	my $server_port = System::nextPort() ;
	my $client_cert = $client_cert_names ? $openssl->file(@$client_cert_names) : undef ;
	my $client_ca = $client_ca_names ? $openssl->file(@$client_ca_names) : undef ;
	my $server_cert = $server_cert_names ? $openssl->file(@$server_cert_names) : undef ;
	my $server_ca = $server_ca_names ? $openssl->file(@$server_ca_names) : undef ;
	my $spool_dir = System::createSpoolDir() ;
	System::submitSmallMessage( $spool_dir ) ;
	my $emailrelay = new Server( undef , undef , undef , $spool_dir , undef , $client_cert , $client_ca ) ;
	$emailrelay->set_dst( "$localhost:$server_port" ) ;
	my $admin_client = new AdminClient( $emailrelay->adminPort() , $localhost ) ;
	Check::ok( $emailrelay->run( \%args ) , "failed to start emailrelay" ) ;
	Check::ok( $admin_client->open() , "cannot connect for admin" ) ;
	my $client_log = $emailrelay->stderr() ;

	# test
	my $server_log = System::tempfile( "sserver" ) ;
	$openssl->runServer( $server_port , $server_log , $server_cert , $server_ca ,
		sub { Server::sleep_cs(20) ; $admin_client->doTerminate() ; kill 15 , $_[0] } ) ;
	if( $expect_failure )
	{
		Check::fileDoesNotContain( $client_log , "tls.*established" ) ;
		Check::fileContains( $client_log , "tls error" ) ;
		Check::fileDoesNotContain( $client_log , "BEGIN CERT" ) ;
		Check::fileContains( $server_log , "ERROR" ) ;
	}
	else
	{
		Check::fileContains( $client_log , "tls.*established" ) ;
		Check::fileDoesNotContain( $client_log , "tls error" ) ;
		Check::fileContains( $client_log , "BEGIN CERT" ) ;
	}

	# tear down
	$emailrelay->cleanup() ;
	$openssl->cleanup() ;
	System::deleteSpoolDir( $spool_dir , 1 ) ;
	System::unlink( $client_log ) ;
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
	my $client_cert = $client_cert_names ? $openssl->file(@$client_cert_names) : undef ;
	my $client_ca = $client_ca_names ? $openssl->file(@$client_ca_names) : undef ;
	my $server_cert = $server_cert_names ? $openssl->file(@$server_cert_names) : undef ;
	my $server_ca = $server_ca_names ? $openssl->file(@$server_ca_names) : undef ;
	my $client_spool_dir = System::createSpoolDir() ;
	my $server_spool_dir = System::createSpoolDir() ;
	System::submitSmallMessage( $client_spool_dir ) ;
	my $client = new Server( undef , undef , undef , $client_spool_dir , undef , $client_cert , $client_ca ) ;
	my $server = new Server( $server_port , undef , undef , $server_spool_dir , undef , $server_cert , $server_ca ) ;
	$client->set_dst( "$localhost:$server_port" ) ;
	Check::ok( $server->run( \%server_args ) , "failed to start emailrelay as server" ) ;
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
		Check::fileDoesNotContain( $client_log , "tls.*established" ) ;
		System::waitForFileLine( $server_log , "tls error" ) ;
		Check::fileDoesNotContain( $server_log , "tls.*established" ) ;
	}
	else
	{
		System::waitForFiles( $server->spoolDir()."/emailrelay.*.envelope" , 1 ) ;
		System::waitForFileLine( $client_log , "tls.*established" ) ;
		Check::fileDoesNotContain( $client_log , "tls error" ) ;
		Check::fileContains( $client_log , "BEGIN CERT" ) ;
		System::waitForFileLine( $server_log , "tls.*established" ) ;
		Check::fileDoesNotContain( $server_log , "tls error" ) ;
	}

	# tear down
	$server->kill() ;
	$client->kill() ;
	$client->cleanup() ;
	$server->cleanup() ;
	System::deleteSpoolDir( $client_spool_dir , 1 ) ;
	System::deleteSpoolDir( $server_spool_dir , 1 ) ;
	System::unlink( $client_log ) ;
	System::unlink( $server_log ) ;
	$openssl->cleanup() ;
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
		}
		else
		{
			print $System::verbose ? "$name: failed: $error\n\n" : "failed: $error\n" ;
			$run_all_ok = 0 ;
			if( ! opt_keep_going )
			{
				return 0 ;
			}
		}
	}
	else
	{
		print $System::verbose ? "$name: passed\n\n" : "ok\n" ;
	}
	return 1 ;
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

my %run_tests = () ;
my %skip_tests = () ;
my $run_tests = 0 ;
for my $arg ( @ARGV )
{
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
for my $test ( @tests )
{
	if( ( $run_tests && !exists $run_tests{$test} ) || $skip_tests{$test} )
	{
		print "(skipping $test)\n" ;
	}
	else
	{
		last if !run( $test ) ;
	}
}

if( !$run_all_ok )
{
	print "failed\n" ;
	cleanup() ;
	exit 1 ;
}

sub cleanup
{
	for my $pid ( @Server::pid_list , @Helper::pid_list , @TestServer::pid_list )
	{
		if( defined($pid) && $pid > 0 )
		{
			#print "killing $pid\n" ;
			kill 15 , $pid ;
		}
	}
}

END
{
	cleanup() ;
}

exit( exists($opts{t}) ? 1 : 0 ) ; # prevent .sh cleanup if -t

