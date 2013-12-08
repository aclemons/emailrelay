#!/usr/bin/perl
#
# Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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

use strict ;
use FileHandle ;
use Scanner ;
use Verifier ;
use System ;

package Server ;

our @pid_list = () ;
our $bin_dir = ".." ;
my $exe_name = "emailrelay" ;

sub new
{
	my ( $classname , $smtp_port , $pop_port , $admin_port , $spool_dir , $tmp_dir ) = @_ ;

	$smtp_port = defined($smtp_port) ? $smtp_port : System::nextPort() ;
	$pop_port = defined($pop_port) ? $pop_port : System::nextPort() ;
	$admin_port = defined($admin_port) ? $admin_port : System::nextPort() ;
	my $scanner_port = System::nextPort() ;
	my $verifier_port = System::nextPort() ;

	my %me = (
		m_exe => System::exe( $bin_dir , $exe_name ) ,
		m_smtp_port => $smtp_port ,
		m_pop_port => $pop_port ,
		m_admin_port => $admin_port ,
		m_rc => undef ,
		m_stdout => System::tempfile("stdout",$tmp_dir) ,
		m_stderr => System::tempfile("stderr",$tmp_dir) ,
		m_log_file => undef ,
		m_pidfile => System::tempfile("pidfile",$tmp_dir) ,
		m_pid => undef ,
		m_pop_secrets => System::tempfile("pop.auth",$tmp_dir) ,
		m_client_secrets => System::tempfile("client.auth",$tmp_dir) ,
		m_server_secrets => System::tempfile("server.auth",$tmp_dir) ,
		m_poll_timeout => 1 ,
		m_dst => "dummy:25" ,
		m_spool_dir => (defined($spool_dir)?$spool_dir:System::createSpoolDir(undef,$tmp_dir)) ,
		m_user => "nobody" ,
		m_full_command => undef ,
		m_filter => System::tempfile("filter",$tmp_dir) . ( System::unix() ? "" : ".js" ) ,
		m_client_filter => System::tempfile("client-filter",$tmp_dir) . ( System::unix() ? "" : ".js" ) ,
		m_scanner_port => $scanner_port ,
		m_verifier_port => $verifier_port ,
		m_max_size => 1000 ,
	) ;
	$me{m_log_file} = $me{m_stderr} if !System::unix() ;
	my $this = bless \%me , $classname ;
	$this->_check() ;
	return $this ;
}

sub exe { return shift->{'m_exe'} }
sub set_exe { $_[0]->{'m_exe'} = $_[1] }
sub smtpPort { return shift->{'m_smtp_port'} }
sub adminPort { return shift->{'m_admin_port'} }
sub scannerPort { return shift->{'m_scanner_port'} }
sub scannerAddress { return "net:localhost:" . shift->{'m_scanner_port'} }
sub verifierPort { return shift->{'m_verifier_port'} }
sub verifierAddress { return "net:localhost:" . shift->{'m_verifier_port'} }
sub popPort { return shift->{'m_pop_port'} }
sub popSecrets { return shift->{'m_pop_secrets'} }
sub clientSecrets { return shift->{'m_client_secrets'} }
sub serverSecrets { return shift->{'m_server_secrets'} }
sub pollTimeout { return shift->{'m_poll_timeout'} }
sub set_pollTimeout { $_[0]->{'m_poll_timeout'} = $_[1] }
sub stdout { return shift->{'m_stdout'} }
sub stderr { return shift->{'m_stderr'} }
sub pidFile { return shift->{'m_pidfile'} }
sub pid { return shift->{'m_pid'} }
sub dst { return shift->{'m_dst'} }
sub set_dst { $_[0]->{'m_dst'} = $_[1] }
sub spoolDir { return shift->{'m_spool_dir'} }
sub set_spoolDir { $_[0]->{'m_spool_dir'} = $_[1] }
sub user { return shift->{'m_user'} }
sub command { return shift->{'m_full_command'} }
sub filter { return shift->{'m_filter'} }
sub clientFilter { return shift->{'m_client_filter'} }
sub maxSize { return shift->{'m_max_size'} }
sub rc { return shift->{'m_rc'} }
sub logFile { return shift->{'m_log_file'} }

sub _check
{
	my ( $this ) = @_ ;
	if( ! -x $this->exe() )
	{
		die "invalid server executable [".$this->exe()."]" ;
	}
}

sub _pid
{
	my ( $this ) = @_ ;
	my $fh = new FileHandle( $this->pidFile() ) ;
	return undef if !$fh ;
	my $line = <$fh> ;
	chomp $line ;
	return $line ;
}

sub canRun
{
	# Returns true if all the required ports are free.
	my ( $this , $port_list_ref ) = @_ ;
	my @port_list = defined($port_list_ref) ? @$port_list_ref : Port::list() ;
	return
		Port::isFree($this->smtpPort(),@port_list) &&
		Port::isFree($this->adminPort(),@port_list) &&
		Port::isFree($this->popPort(),@port_list) ;
}

sub canDo
{
	# Returns true if built with the relevant functionality.
	my ( $this , $type , $default_ ) = @_ ;
	return 1 if !System::unix() ;
	local $/ ;
	my $fh = new FileHandle( $this->exe() . " --version --verbose |" ) ;
	my $output = <$fh> ;
	my $has_enable = $output =~ m/\[.*enable_$type.*\]/m ;
	my $has_disable = $output =~ m/\[.*disable_$type.*\]/m ;
	return $has_enable ? 1 : ( $has_disable ? 0 : $default_ ) ;
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
		( exists($sw{Log}) ? "--log " : "" ) .
		( exists($sw{LogTime}) ? "--log-time " : "" ) .
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
		( exists($sw{Verifier}) ? "--verifier __VERIFIER__ " : "" ) .
		( exists($sw{DontServe}) ? "--dont-serve " : "" ) .
		( exists($sw{ClientAuth}) ? "--client-auth __CLIENT_SECRETS__ " : "" ) .
		( exists($sw{MaxSize}) ? "--size __MAX_SIZE__ " : "" ) .
		( exists($sw{ServerSecrets}) ? "--server-auth __SERVER_SECRETS__ " : "" ) .
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
	$command_tail = _set( $command_tail , "__SCANNER__" , $this->scannerAddress() ) ;
	$command_tail = _set( $command_tail , "__VERIFIER__" , $this->verifierAddress() ) ;
	$command_tail = _set( $command_tail , "__CLIENT_SECRETS__" , $this->clientSecrets() ) ;
	$command_tail = _set( $command_tail , "__MAX_SIZE__" , $this->maxSize() ) ;
	$command_tail = _set( $command_tail , "__SERVER_SECRETS__" , $this->serverSecrets() ) ;

	my $valgrind = "" ;
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
	System::log_( "[$full]" ) ;
	if( defined($gtest) ) { $main::ENV{G_TEST} = $gtest }
	my $rc = system( $full ) ;
	if( defined($gtest) ) { $main::ENV{G_TEST} = "xx" }

	my $ok = $rc >= 0 && ($rc & 127) == 0 ;
	$this->{'m_rc'} = $rc ;
	if( $ok && defined($switches_ref->{PidFile}) )
	{
		# wait for the pid file to be written
		for( my $i = 0 ; $i < 200 ; $i++ )
		{
			sleep_cs() ;
			if( -f $this->pidFile() ) { next }
		}
		my $pid = $this->_pid() ;
		$ok = defined($pid) ;
		$this->{'m_pid'} = $pid ;
	}
	push @pid_list , $this->{'m_pid'} ;
	return $ok ;
}

sub message
{
	# Returns the first warning or error from the server's log file.
	my ( $this ) = @_ ;
	my $err = new FileHandle($this->stderr()) ;
	while( <$err> )
	{
		my $line = $_ ;
		chomp $line ;
		if( $line =~ m/warning:/ || $line =~ m/error:/ || $line =~ m/exception:/ )
		{
			return $line ;
		}
	}
	return undef ;
}

sub sleep_cs
{
	# Sleeps for a number of centiseconds.
	my ( $cs ) = @_ ;
	$cs = defined($cs) ? $cs : 1 ;
	select( undef , undef , undef , 0.01 * $cs ) ;
}

sub wait
{
	# Waits to die
	my ( $this , $timeout_cs ) = @_ ;
	System::wait( $this->pid() , $timeout_cs ) ;
}

sub kill
{
	# Kills the server and waits for it to die.
	my ( $this , $signal__not_used , $timeout_cs ) = @_ ;
	System::kill_( $this->pid() , $timeout_cs ) ;
	System::wait( $this->pid() , $timeout_cs ) ;
}

sub cleanup
{
	# Cleans up some files.
	my ( $this ) = @_ ;
	unlink( $this->stdout() ) ;
	unlink( $this->stderr() ) ;
	unlink( $this->popSecrets() ) ;
	unlink( $this->clientSecrets() ) ;
	unlink( $this->serverSecrets() ) ;
	unlink( $this->filter() ) ;
	unlink( $this->clientFilter() ) ;
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
		$exe = System::weirdpath( $exe ) ;
		$main::ENV{G_TEST} = "special-exit-code" ;
		my $rc = system( "$exe --version --verbose --hidden" ) ;
		$main::ENV{G_TEST} = "xx" ;
		return ( ( $rc >> 8 ) & 255 ) == 23 ;
	}
}

1 ;

