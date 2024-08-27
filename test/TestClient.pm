#!/usr/bin/perl
#
# Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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
# TestClient.pm
#
# A wrapper for running the "emailrelay_test_client" program.
#
# See also: emailrelay_test_client.cpp
#
# Synopsis:
#
#	use TestClient ;
#	$TestClient::bin_dir = "." ;
#	my $tc = new TestClient( 10025 , "127.0.0.1" , "-v --utf8" ) ;
#	print $tc->command() , "\n" ;
#	print $tc->logfile() , "\n" ;
#	my $ec = $tc->run() ;
#	$tc->cleanup() ;
#

use strict ;
use System ;

package TestClient ;

our $bin_dir = "." ;

sub new
{
	my ( $classname , $port , $server , $options ) = @_ ;

	$port ||= 10025 ;
	$server ||= $System::localhost ; # IP address, not domain name

	my $exe = System::exe( $bin_dir , "emailrelay_test_client" ) ;
	if( ! -x $exe )
	{
		die "invalid test client executable [$exe]" ;
	}

	return bless {
		m_options => $options ,
		m_server => $server ,
		m_port => $port ,
		m_exe => $exe ,
		m_logfile => System::tempfile("test-client.out") ,
		m_iterations => 1 ,
		m_connections => 1 ,
		m_messages => 1 ,
		m_recipients => 1 ,
		m_lines => 100 ,
		m_line_length => 100 ,
		m_timeout => 10 ,
	} , $classname ;
}

sub exe { return shift->{m_exe} }
sub server { return shift->{m_port} }
sub port { return shift->{m_port} }
sub logfile { return shift->{m_logfile} }

sub command
{
	my ( $this ) = @_ ;
	return System::sanepath($this->exe()) . " " . join(" ",
		$this->{m_options} ,
		"--iterations" , $this->{m_iterations} ,
		"--connections" , $this->{m_connections} ,
		"--messages" , $this->{m_messages} ,
		"--recipients" , $this->{m_connections} ,
		"--lines" , $this->{m_lines} ,
		"--line-length" , $this->{m_line_length} ,
		"--timeout" , $this->{m_timeout} ,
		$this->{m_server} ,
		$this->{m_port} ) ;
}

sub run
{
	my ( $this ) = @_ ;
	my $log = $this->{m_logfile} ;
	my $cmd = $this->command() ;
	my $full_cmd = System::commandline( $cmd , { stdout => $log , stderr => $log } ) ;
	System::log_( "running [$full_cmd]" ) ;
	return system( $full_cmd ) ;
}

sub cleanup
{
	my ( $this ) = @_ ;
	System::unlink( $this->{m_logfile} ) ;
}

1 ;
