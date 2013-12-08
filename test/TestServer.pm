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
# TestServer.pm
#
# A wrapper for running the "emailrelay-test-server" program.
#

use strict ;
use FileHandle ;
use Scanner ;
use System ;
use Port ;

package TestServer ;

our @pid_list = () ;
our $bin_dir = "." ;

sub new
{
	my ( $classname , $port ) = @_ ;

	$port = defined($port) ? $port : 10025 ;

	my %me = (
		m_port => $port ,
		m_exe => System::exe( $bin_dir , "emailrelay-test-server" ) ,
		m_logfile => "$bin_dir/.tmp.emailrelay-test-server.out" ,
		m_pidfile => "$bin_dir/.emailrelay-test-server.pid" ,
		m_pid => undef ,
	) ;
	my $this = bless \%me , $classname ;
	$this->_check() ;
	return $this ;
}

sub exe { return shift->{'m_exe'} }
sub pid { return shift->{'m_pid'} }
sub port { return shift->{'m_port'} }

sub _check
{
	my ( $this ) = @_ ;
	if( ! -x $this->exe() )
	{
		die "invalid test server executable [".$this->exe()."]" ;
	}
	if( ! Port::isFree($this->port(),Port::list()) )
	{
		die "port " . $this->port() . " not free" ;
	}
	unlink( $this->{m_pidfile} ) if -f $this->{m_pidfile} ;
	if( -f $this->{m_pidfile} )
	{
		die "cannot remove old pidfile [".$this->{m_pidfile}."]" ;
	}
}

sub run
{
	my ( $this , $sw , $wait_cs ) = @_ ;
	$sw = "" if !defined($sw) ;
	$wait_cs ||= 50 ;
	my $log = $this->{'m_logfile'} ;

	my $cmd = System::weirdpath($this->exe()) . " --port " . $this->port() . " $sw" ;
	my $full_cmd = System::commandline( $cmd , { stdout => $log , stderr => $log , background => 1 } ) ;
	system( $full_cmd ) ;

	# wait for the pid file to be written
	for( my $i = 0 ; $i < $wait_cs ; $i++ )
	{
		select( undef , undef , undef , 0.01 ) ;
		my $fh = new FileHandle( $this->{'m_pidfile'} ) ;
		next if !$fh ;
		my $line = <$fh> ;
		chomp $line ;
		if( $line > 0 )
		{
			my $pid = $line ;
			$this->{'m_pid'} = $pid ;
			push @pid_list , $pid ;
			last ;
		}
	}

	return System::processIsRunning( $this->{'m_pid'} ) ;
}

sub kill
{
	my ( $this ) = @_ ;
	if( defined($this->pid()) )
	{
		System::kill_( $this->pid() ) ;
		System::sleep_cs( 10 ) ;
	}
}

sub cleanup
{
	my ( $this ) = @_ ;
	$this->kill() ;
	unlink( $this->{'m_pidfile'} ) ;
	unlink( $this->{'m_logfile'} ) ;
}

1 ;

