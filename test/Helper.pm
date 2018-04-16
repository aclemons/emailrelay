#!/usr/bin/perl
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
# Helper.pm
#
# A wrapper for running test helper programs such as
# "emailrelay-test-scanner".
#
# Synopsis:
#
#	$Helper::bin_dir = "." ;
#	my $helper = new Helper( "emailrelay-test-scanner" ) ;
#	$helper->run() ;
#	$helper->kill() ;
#	open $helper->logfile() ;
#	$helper->cleanup() ;
#	kill 15 , @Helper::pid_list() ;
#

use strict ;
use FileHandle ;
use System ;

package Helper ;

our @pid_list = () ;
our $bin_dir = "." ;

sub new
{
	my ( $classname , $exe , $port ) = @_ ;

	die if !defined($exe) ;
	$port ||= 10010 ;

	( my $short_name = $exe ) =~ s/emailrelay-test-// ;
	$short_name =~ s/\.[a-z]*$// ;

	my $this = bless {
		m_short_name => $short_name ,
		m_exe_name => $exe ,
		m_port => $port ,
		m_logfile => ".tmp.$short_name.out.$$" ,
		m_pidfile => ".tmp.$short_name.pid.$$" ,
		m_pid => undef ,
	} , $classname ;
	$this->_check() ;
	return $this ;
}

sub port
{
	my ( $this ) = @_ ;
	return $this->{'m_port'} ;
}

sub logfile
{
	my ( $this ) = @_ ;
	return $this->{'m_logfile'} ;
}

sub _check
{
	my ( $this ) = @_ ;
	if( ! -x $this->exe() )
	{
		my $name = $this->{m_short_name} ;
		die "no $name executable [".$this->exe()."]" ;
	}
}

sub exe
{
	my ( $this ) = @_ ;
	return System::exe( $bin_dir , $this->{m_exe_name} ) ;
}

sub run
{
	my ( $this ) = @_ ;
	my $port = $this->{'m_port'} ;
	my $pidfile = $this->{'m_pidfile'} ;
	my $logfile = $this->{'m_logfile'} ;
	my $exe = $this->exe() ;
	my $cmd = System::mangledpath($exe) . " --port $port --log --log-file $logfile --debug --pid-file $pidfile" ;
	my $full = System::commandline( $cmd , { background => 1 } ) ;
	System::log_( "[$full]" ) ;
	system( $full ) ;
	for( my $i = 0 ; $i < 200 ; $i++ )
	{
		System::sleep_cs( 2 ) ;
		if( -f $pidfile ) { next }
	}
	my $f = new FileHandle("< $pidfile") ;
	my $line = <$f> ; chomp $line ;
	$this->{'m_pid'} = $line ;
	push @pid_list , $this->{'m_pid'} ;
}

sub pid
{
	my ( $this ) = @_ ;
	return $this->{'m_pid'} ;
}

sub kill
{
	# kill and wait to die
	my ( $this , $signal__not_used , $timeout_cs ) = @_ ;
	System::kill_( $this->pid() , $timeout_cs ) ;
	System::waitpid( $this->pid() ) ;
}

sub cleanup
{
	my ( $this ) = @_ ;
	System::unlink( $this->{'m_pidfile'} ) ;
	System::unlink( $this->{'m_logfile'} ) ;
}

1 ;
