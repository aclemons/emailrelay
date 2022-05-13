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
# Helper.pm
#
# A wrapper for running test helper programs such as
# "emailrelay-test-scanner" that use command-line
# options such as "--port", "--log-file", "--pid-file"
# etc.
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

	my $exe_path = System::exe( $bin_dir , $exe ) ;
	die "no [$short_name] exectuable: [$exe_path]"
		if ! -x $exe_path ;

	return bless {
		m_short_name => $short_name ,
		m_exe_path => $exe_path ,
		m_port => $port ,
		m_logfile => System::tempfile("$short_name.out") ,
		m_pidfile => System::tempfile("$short_name.pid") ,
		m_pid => undef ,
	} , $classname ;
}

sub port
{
	my ( $this ) = @_ ;
	return $this->{m_port} ;
}

sub logfile
{
	my ( $this ) = @_ ;
	return $this->{m_logfile} ;
}

sub exe
{
	my ( $this ) = @_ ;
	return $this->{m_exe_path} ;
}

sub run
{
	my ( $this ) = @_ ;
	my $port = $this->{m_port} ;
	my $pidfile = $this->{m_pidfile} ;
	my $logfile = $this->{m_logfile} ;
	my $exe = $this->exe() ;
	my $cmd = System::sanepath($exe) . " --port $port --log --log-file $logfile --debug --pid-file $pidfile" ;
	my $full = System::commandline( $cmd , { background => 1 } ) ;
	System::log_( "running [$full]" ) ;
	system( $full ) ;
	my $pid = System::waitForPid( $pidfile ) ;
	die "no pidfile created by [$exe]" if !$pid ;
	$this->{m_pid} = $pid ;
	push @pid_list , $this->{m_pid} ;
}

sub pid
{
	my ( $this ) = @_ ;
	return $this->{m_pid} ;
}

sub kill
{
	# kill and wait to die
	my ( $this ) = @_ ;
	System::kill_( $this->pid() ) ;
}

sub cleanup
{
	my ( $this ) = @_ ;
	System::unlink( $this->{m_pidfile} ) ;
	System::unlink( $this->{m_logfile} ) ;
}

1 ;
