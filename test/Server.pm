#!/usr/bin/perl
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
# Server.pm
#
# Runs the emailrelay program as a server.
#
# Synopsis:
#
#	$Server::bin_dir = "." ;
#	my $server = new Server( 10025 , 10101 , 10026 , "/var/tmp" ) ;
#	$server->run( { AsServer => 1 , ForwardTo => "localhost:10020" ... } ) ;
#	open $server->logFile() ... ;
#	$server->kill() ;
#	$server->cleanup() ;
#	kill 15 , @Server::pid_list ;
#

use strict ;
use FileHandle ;
use File::Basename ;
use System ;

package Server ;

our @pid_list = () ;
our $bin_dir = ".." ;
our $tls_config = "x" ;
our $with_valgrind = undef ;
my $exe_name = "emailrelay" ;

sub new
{
	my ( $classname , $smtp_port , $pop_port , $admin_port , $spool_dir , $tls_certificate , $tls_verify ) = @_ ;

	my $exe = System::exe( $bin_dir , $exe_name ) ;
	if( !-x $exe ) { die "invalid server executable [$exe]" }

	$spool_dir ||= System::createSpoolDir() ;
	$smtp_port ||= System::nextPort() ;
	$pop_port ||= System::nextPort() ;
	$admin_port ||= System::nextPort() ;

	my $scanner_port = System::nextPort() ;
	my $verifier_port = System::nextPort() ;
	my $stdout = System::tempfile( "stdout" ) ;
	my $stderr = System::tempfile( "stderr" ) ;
	my $piddir = System::createPidDir( System::tempfile("piddir") ) ;
	my $pidfile = "$piddir/pid" ;

	my $filter = System::tempfile("filter") ;
	$filter .= ".js" if System::windows() ;

	my $client_filter = System::tempfile("client-filter") ;
	$client_filter .= ".js" if System::windows() ;

	return bless {
		m_exe => $exe ,
		m_smtp_port => $smtp_port ,
		m_pop_port => $pop_port ,
		m_admin_port => $admin_port ,
		m_rc => undef ,
		m_stdout => $stdout ,
		m_stderr => $stderr ,
		m_log_file => (System::unix()?undef:$stderr) ,
		m_piddir => $piddir ,
		m_pidfile => $pidfile ,
		m_pid => undef ,
		m_pop_secrets => System::tempfile("pop.auth") ,
		m_client_secrets => System::tempfile("client.auth") ,
		m_server_secrets => System::tempfile("server.auth") ,
		m_tls_certificate => $tls_certificate ,
		m_tls_verify => $tls_verify ,
		m_tls_config => $tls_config ,
		m_poll_timeout => 1 ,
		m_dst => "dummy3450930958349:25" ,
		m_spool_dir => $spool_dir ,
		m_user => "nobody" ,
		m_full_command => undef ,
		m_filter => $filter ,
		m_client_filter => $client_filter ,
		m_scanner_address => "${System::localhost}:${scanner_port}" ,
		m_verifier_port => $verifier_port ,
		m_max_size => 1000 ,
	} , $classname ;
}

sub exe { return shift->{m_exe} }
sub set_exe { $_[0]->{m_exe} = $_[1] }
sub smtpPort { return shift->{m_smtp_port} }
sub adminPort { return shift->{m_admin_port} }
sub scannerAddress { return shift->{m_scanner_address} }
sub set_scannerAddress { $_[0]->{m_scanner_address} = $_[1] }
sub verifierPort { return shift->{m_verifier_port} }
sub verifierAddress { return "net:$System::localhost:" . shift->{m_verifier_port} }
sub popPort { return shift->{m_pop_port} }
sub popSecrets { return shift->{m_pop_secrets} }
sub clientSecrets { return shift->{m_client_secrets} }
sub serverSecrets { return shift->{m_server_secrets} }
sub tlsCertificate { return shift->{m_tls_certificate} }
sub tlsVerify { return shift->{m_tls_verify} }
sub tlsConfig { return shift->{m_tls_config} }
sub pollTimeout { return shift->{m_poll_timeout} }
sub set_pollTimeout { $_[0]->{m_poll_timeout} = $_[1] }
sub stdout { return shift->{m_stdout} }
sub stderr { return shift->{m_stderr} }
sub pidFile { return shift->{m_pidfile} }
sub pid { return shift->{m_pid} }
sub dst { return shift->{m_dst} }
sub set_dst { $_[0]->{m_dst} = $System::localhost . ":" . $_[1] }
sub spoolDir { return shift->{m_spool_dir} }
sub set_spoolDir { $_[0]->{m_spool_dir} = $_[1] }
sub user { return shift->{m_user} }
sub command { return shift->{m_full_command} }
sub filter { return shift->{m_filter} }
sub clientFilter { return shift->{m_client_filter} }
sub maxSize { return shift->{m_max_size} }
sub rc { return shift->{m_rc} }
sub logFile { return shift->{m_log_file} }

sub _pid
{
	my ( $this ) = @_ ;
	my $fh = new FileHandle( $this->pidFile() , "r" ) ;
	return undef if !$fh ;
	my $line = <$fh> ;
	chomp $line ;
	return $line ;
}

sub _set
{
	my ( $s , $var , $value ) = @_ ;
	$s =~ s/$var/$value/g ;
	return $s ;
}

sub _switches
{
	my ( %sw ) = @_ ;

	return
		"" .
		( exists($sw{AsServer}) ? "--as-server " : "" ) .
		( exists($sw{Log}) ? "--log --log-time --log-address " : "" ) .
		( exists($sw{Port}) ? "--port __SMTP_PORT__ " : "" ) .
		( exists($sw{Admin}) ? "--admin __ADMIN_PORT__ " : "" ) .
		( exists($sw{Pop}) ? "--pop " : "" ) .
		( exists($sw{PopPort}) ? "--pop-port __POP_PORT__ " : "" ) .
		( exists($sw{PopAuth}) ? "--pop-auth __POP_SECRETS__ " : " " ) .
		( exists($sw{PidFile}) ? "--pid-file __PID_FILE__ " : "" ) .
		( exists($sw{SpoolDir}) ? "--spool-dir __SPOOL_DIR__ " : "" ) .
		( exists($sw{Syslog}) ? "--syslog " : "" ) .
		( exists($sw{AdminTerminate}) ? "--admin-terminate " : "" ) .
		( exists($sw{Help}) ? "--help " : "" ) .
		( exists($sw{Verbose}) ? "--verbose " : "" ) .
		( exists($sw{Forward}) ? "--forward " : "" ) .
		( exists($sw{ForwardTo}) ? "--forward-to __FORWARD_TO__ " : "" ) .
		( exists($sw{ForwardToSome}) ? "--forward-to-some " : "" ) .
		( exists($sw{User}) ? "--user __USER__ " : "" ) .
		( exists($sw{Debug}) ? "--debug " : "" ) .
		( !System::unix() && exists($sw{Log}) ? "--log-file __LOG_FILE__ " : "" ) .
		( exists($sw{NoDaemon}) ? "--no-daemon " : "" ) .
		( exists($sw{Hidden}) && !System::unix() ? "--hidden " : "" ) .
		( exists($sw{NoSmtp}) ? "--no-smtp " : "" ) .
		( exists($sw{Poll}) ? "--poll __POLL_TIMEOUT__ " : "" ) .
		( exists($sw{Filter}) ? "--filter __FILTER__ " : "" ) .
		( exists($sw{FilterTimeout}) ? "--filter-timeout 1 " : "" ) .
		( exists($sw{ConnectionTimeout}) ? "--connection-timeout 1 " : "" ) .
		( exists($sw{ClientFilter}) ? "--client-filter __CLIENT_FILTER__ " : "" ) .
		( exists($sw{Immediate}) ? "--immediate " : "" ) .
		( exists($sw{Scanner}) ? "--filter __SCANNER__ " : "" ) .
		( exists($sw{Verifier}) ? "--address-verifier __VERIFIER__ " : "" ) .
		( exists($sw{DontServe}) ? "--dont-serve " : "" ) .
		( exists($sw{ClientAuth}) ? "--client-auth __CLIENT_SECRETS__ " : "" ) .
		( exists($sw{MaxSize}) ? "--size __MAX_SIZE__ " : "" ) .
		( exists($sw{ServerSecrets}) ? "--server-auth __SERVER_SECRETS__ " : "" ) .
		( exists($sw{Domain}) ? "--domain test.localnet " : "" ) .
		( exists($sw{ServerTls}) ? "--server-tls " : "" ) .
		( exists($sw{ServerTlsRequired}) ? "--server-tls-required " : "" ) .
		( exists($sw{ClientTls}) ? "--client-tls " : "" ) .
		( exists($sw{ClientTlsConnection}) ? "--client-tls-connection " : "" ) .
		( exists($sw{ServerTlsCertificate}) ? "--server-tls-certificate __TLS_CERTIFICATE__ " : "" ) .
		( (exists($sw{ServerTlsVerify}) && $sw{ServerTlsVerify}) ? "--server-tls-verify __TLS_VERIFY__ " : "" ) .
		( exists($sw{ClientTlsCertificate}) ? "--client-tls-certificate __TLS_CERTIFICATE__ " : "" ) .
		( (exists($sw{ClientTlsVerify}) && $sw{ClientTlsVerify}) ? "--client-tls-verify __TLS_VERIFY__ " : "" ) .
		( exists($sw{TlsConfig}) ? "--tls-config=__TLS_CONFIG__ " : "" ) .
		"" ;
}

sub _set_all
{
	my ( $this , $command_tail ) = @_ ;

	$command_tail = defined($command_tail) ? $command_tail : "" ;

	$command_tail = _set( $command_tail , "__SMTP_PORT__" , $this->smtpPort() ) ;
	$command_tail = _set( $command_tail , "__ADMIN_PORT__" , $this->adminPort() ) ;
	$command_tail = _set( $command_tail , "__POP_PORT__" , $this->popPort() ) ;
	$command_tail = _set( $command_tail , "__POP_SECRETS__" , $this->popSecrets() ) ;
	$command_tail = _set( $command_tail , "__PID_FILE__" , $this->pidFile() ) ;
	$command_tail = _set( $command_tail , "__FORWARD_TO__" , $this->dst() ) ;
	$command_tail = _set( $command_tail , "__LOG_FILE__" , $this->logFile() ) ;
	$command_tail = _set( $command_tail , "__SPOOL_DIR__" , $this->spoolDir() ) ;
	$command_tail = _set( $command_tail , "__USER__" , $this->user() ) ;
	$command_tail = _set( $command_tail , "__POLL_TIMEOUT__" , $this->pollTimeout() ) ;
	$command_tail = _set( $command_tail , "__FILTER__" , $this->filter() ) ;
	$command_tail = _set( $command_tail , "__CLIENT_FILTER__" , $this->clientFilter() ) ;
	$command_tail = _set( $command_tail , "__SCANNER__" , "net:" . $this->scannerAddress() ) ;
	$command_tail = _set( $command_tail , "__VERIFIER__" , $this->verifierAddress() ) ;
	$command_tail = _set( $command_tail , "__CLIENT_SECRETS__" , $this->clientSecrets() ) ;
	$command_tail = _set( $command_tail , "__MAX_SIZE__" , $this->maxSize() ) ;
	$command_tail = _set( $command_tail , "__SERVER_SECRETS__" , $this->serverSecrets() ) ;
	$command_tail = _set( $command_tail , "__TLS_CERTIFICATE__" , $this->tlsCertificate() ) ;
	$command_tail = _set( $command_tail , "__TLS_VERIFY__" , $this->tlsVerify() ) ;
	$command_tail = _set( $command_tail , "__TLS_CONFIG__" , $this->tlsConfig() ) ;

	my $valgrind = $with_valgrind ? "valgrind -q " : "" ;
	return $valgrind . $this->exe() . " " .  $command_tail ;
}

sub run
{
	# Starts the server and waits for a pid file to be created.
	my ( $this , $switches_ref , $sudo_prefix , $gtest , $background ) = @_ ;

	if(!defined($background)) { $background = System::unix() ? 0 : 1 }

	my $command_with_switches = $this->_set_all(_switches(%$switches_ref)) ;

	my $full = System::commandline( $command_with_switches , {
			background => $background ,
			stdout => $this->stdout() ,
			stderr => $this->stderr() ,
			prefix => $sudo_prefix ,
			gtest => $gtest ,
		} ) ;

	$this->{'m_full_command'} = $full ;
	System::log_( "running [$full]" ) ;
	if( defined($gtest) ) { $main::ENV{G_TEST} = $gtest }
	my $rc = system( $full ) ;
	if( defined($gtest) ) { $main::ENV{G_TEST} = "xx" }
	$this->{'m_rc'} = $rc ;

	# check for an immediate startup error
	my $error = $this->error() ;
	if( $error )
	{
		System::log_( "error in the log file [".File::Basename::basename($this->stderr())."]: [$error]" ) ;
		return undef ;
	}

	# read the pid from the pid-file
	if( defined($switches_ref->{PidFile}) )
	{
		# wait for the pid file to be written
		my $pid = System::waitForPid( $this->pidFile() ) ;
		$this->{'m_pid'} = $pid ;
		push @pid_list , $this->{'m_pid'} ;
	}

	my $ok = $rc >= 0 && ($rc & 127) == 0 ;
	if( !$ok )
	{
		System::log_( "non-zero exit code: $rc" ) ;
		return undef ;
	}
	return 1 ;
}

sub error
{
	# Returns the first error from the server's log file. As a
	# required feature this only returns errors with a timestamp.
	my ( $this ) = @_ ;
	my $fh = new FileHandle( $this->stderr() ) or return undef ;
	while(<$fh>)
	{
		chomp( my $line = $_ ) ;
		my ( $error ) = ( $line =~ m/^(?:emailrelay: [0123456789.]*: error: )(.*)/ ) ;
		return $error if $error ;
	}
	return undef ;
}

sub message
{
	# Returns the first warning or error from the server's log file.
	my ( $this ) = @_ ;
	my $fh = new FileHandle( $this->stderr() ) or return undef ;
	while(<$fh>)
	{
		chomp( my $line = $_ ) ;
		return $line
			if( $line =~ m/: warning:/ || $line =~ m/: error:/ || $line =~ m/: exception:/ ) ;
	}
	return undef ;
}

sub wait
{
	# Waits for the server to exit. This does not work well for synchronising the
	# exit of a short-lived 'client-like' process because it requires a pid-file
	# to provide the pid but the pid reading in run() is racy.
	my ( $this ) = @_ ;
	if( $this->pidFile() && $this->pid() )
	{
		System::waitpid( $this->pid() ) ;
	}
}

sub kill
{
	# Kills the server and waits for it to die.
	my ( $this ) = @_ ;
	System::kill_( $this->pid() ) ;
}

sub cleanup
{
	# Cleans up some files.
	my ( $this ) = @_ ;
	System::unlink( $this->stdout() ) ;
	System::unlink( $this->stderr() ) ;
	System::unlink( $this->popSecrets() ) ;
	System::unlink( $this->clientSecrets() ) ;
	System::unlink( $this->serverSecrets() ) ;
	System::unlink( $this->filter() ) ;
	System::unlink( $this->clientFilter() ) ;
	# System::unlink( $this->tlsCertificate() ) ; # done in Openssl::cleanup()
	System::unlink( $this->pidFile() ) if System::windows() ;
	System::deleteSpoolDir( $this->spoolDir() , 1 ) ;
	System::rmdir_( $this->{m_piddir} ) ;
}

sub hasDebug
{
	# Returns true if the executable has debugging code
	# and extra test features built in.

	my ( $this ) = @_ ;
	my $exe = $this->exe() ;
	if( System::unix() )
	{
		my $rc = system( "strings \"$exe\" | fgrep -q 'G_TEST'" ) ;
		return $rc == 0 ;
	}
	else
	{
		$exe = System::sanepath( $exe ) ;
		$main::ENV{G_TEST} = "special-exit-code" ;
		my $rc = system( "$exe --version --verbose --hidden" ) ;
		$main::ENV{G_TEST} = "xx" ;
		my $exit = ( ( $rc >> 8 ) & 255 ) ;
		return $exit == 23 || $exit == 25 ;
	}
}

sub hasThreads
{
	# Returns true if the executable has multi-threading support.

	my ( $this ) = @_ ;
	if( System::unix() )
	{
		my $exe = $this->exe() ;
		my $rc = system( "$exe --version --verbose | grep -qi threading:.*enabled" ) ;
		return $rc == 0 ;
	}
	else
	{
		return 1 ; # assume msvc build with threads
	}
}

sub hasTls
{
	# Returns true if the executable has tls support.

	my ( $this ) = @_ ;
	my $exe = $this->exe() ;
	$exe =~ s/\.exe$/-textmode.exe/ if !System::unix() ;
	my $fh = new FileHandle( "$exe --version --verbose |" ) ;
	while(<$fh>)
	{
		chomp( my $line = $_ ) ;
		if( $line =~ m/tls.library:/i )
		{
			return ( $line =~ m/openssl/i ) || ( $line =~ m/mbed/i ) ;
		}
	}
	return undef ;
}

1 ;
