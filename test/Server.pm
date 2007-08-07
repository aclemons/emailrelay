#!/usr/bin/perl
#
# Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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

use strict ;
use FileHandle ;
use Scanner ;
use System ;

package Server ;

our @pid_list = () ;
our $bin_dir = ".." ;

sub new
{
	my ( $classname , $smtp_port , $pop_port , $admin_port , $spool_dir , $tmp_dir ) = @_ ;

	$smtp_port = defined($smtp_port) ? $smtp_port : 10025 ;
	$pop_port = defined($pop_port) ? $pop_port : 10110 ;
	$admin_port = defined($admin_port) ? $admin_port : 10026 ;
	my $scanner_port = Scanner::port() ;

	my %me = (
		m_exe => "$bin_dir/emailrelay" ,
		m_smtp_port => $smtp_port ,
		m_pop_port => $pop_port ,
		m_admin_port => $admin_port ,
		m_rc => undef ,
		m_stdout => System::tempfile("stdout",$tmp_dir) ,
		m_stderr => System::tempfile("stderr",$tmp_dir) ,
		m_pidfile => System::tempfile("pidfile",$tmp_dir) ,
		m_pid => undef ,
		m_pop_secrets => System::tempfile("pop.auth",$tmp_dir) ,
		m_poll_timeout => 1 ,
		m_dst => "dummy:25" ,
		m_spool_dir => (defined($spool_dir)?$spool_dir:System::createSpoolDir(undef,$tmp_dir)) ,
		m_user => "nobody" ,
		m_full_command => undef ,
		m_filter => System::tempfile("filter",$tmp_dir) ,
		m_scanner => "localhost:$scanner_port" ,
	) ;
	my $this = bless \%me , $classname ;
	$this->_check() ;
	return $this ;
}

sub exe { return shift->{'m_exe'} }
sub set_exe { $_[0]->{'m_exe'} = $_[1] }
sub smtpPort { return shift->{'m_smtp_port'} }
sub adminPort { return shift->{'m_admin_port'} }
sub scannerAddress { return shift->{'m_scanner'} }
sub popPort { return shift->{'m_pop_port'} }
sub popSecrets { return shift->{'m_pop_secrets'} }
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
sub rc { return shift->{'m_rc'} }

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
	my $f = new FileHandle( $this->pidFile() ) ;
	my $line = <$f> ;
	chomp $line ;
	return $line ;
}

sub canRun
{
	my ( $this , $port_list_ref ) = @_ ;
	my @port_list = defined($port_list_ref) ? @$port_list_ref : Port::list() ;
	return 
		Port::isFree($this->smtpPort(),@port_list) && 
		Port::isFree($this->adminPort(),@port_list) && 
		Port::isFree($this->popPort(),@port_list) ;
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
		( exists($sw{ForwardTo}) ? "--forward-to __FORWARD_TO__ " : "" ) .
		( exists($sw{User}) ? "--user __USER__ " : "" ) .
		( exists($sw{Debug}) ? "--debug " : "" ) .
		( exists($sw{NoDaemon}) ? "--no-daemon " : "" ) .
		( exists($sw{NoSmtp}) ? "--no-smtp " : "" ) .
		( exists($sw{Poll}) ? "--poll __POLL_TIMEOUT__ " : "" ) .
		( exists($sw{Filter}) ? "--filter __FILTER__ " : "" ) .
		( exists($sw{Immediate}) ? "--immediate " : "" ) .
		( exists($sw{Scanner}) ? "--scanner __SCANNER__ " : "" ) .
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
	$command_tail = _set( $command_tail , "__SPOOL_DIR__" , $this->spoolDir() ) ;
	$command_tail = _set( $command_tail , "__USER__" , $this->user() ) ;
	$command_tail = _set( $command_tail , "__POLL_TIMEOUT__" , $this->pollTimeout() ) ;
	$command_tail = _set( $command_tail , "__FILTER__" , $this->filter() ) ;
	$command_tail = _set( $command_tail , "__SCANNER__" , $this->scannerAddress() ) ;

	return $this->exe() . " " .  $command_tail ;
}

sub run
{
	my ( $this , $switches_ref , $command_prefix , $command_suffix ) = @_ ;

	$command_prefix = defined($command_prefix) ? $command_prefix : "" ;
	$command_suffix = defined($command_suffix) ? $command_suffix : "" ;

	my $command_switches = $this->_set_all(_switches(%$switches_ref)) ;
	my $redirection = " >" . $this->stdout() . " 2>" . $this->stderr() ;
	my $full = $command_prefix . $command_switches . $redirection . $command_suffix ;

	$this->{'m_full_command'} = $full ;
	my $rc = system( $full ) ;

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
	# Returns the first warning or error from the server's log file
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
	my ( $cs ) = @_ ;
	$cs = defined($cs) ? $cs : 1 ;
	select( undef , undef , undef , 0.01 * $cs ) ;
}

sub wait
{
	# wait to die
	my ( $this , $timeout_cs ) = @_ ;
	for( my $i = 0 ; $i < $timeout_cs ; $i++ )
	{
		sleep_cs() ;
		if( kill(0,$this->pid()) == 0 )
		{
			next
		}
	}
}

sub kill
{
	# kill and wait to die
	my ( $this , $signal , $timeout_cs ) = @_ ;
	$signal = defined($signal) ? $signal : 15 ;
	$timeout_cs = defined($timeout_cs) ? $timeout_cs : 100 ;
	kill( $signal , $this->pid() ) ;
	$this->wait( $timeout_cs ) ;
}

sub cleanup
{
	my ( $this ) = @_ ;
	unlink( $this->stdout() ) ;
	unlink( $this->stderr() ) ;
	unlink( $this->popSecrets() ) ;
	unlink( $this->filter() ) ;
}

1 ;

