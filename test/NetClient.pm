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
# NetClient.pm
#
# A helper for AdminClient, PopClient and SmtpClient.
#
# Synopsis:
#
#	$nc = new NetClient( "25" , "localhost" , 10 , "OK>" ) ;
#	$nc->send( $tx ) ;
#	$rx = $nc->read() ;
#	$rsp = $nc->cmd( $req ) ;
#   $nc->close() ;
#

use strict ;
use IO::Socket ;
use IO::Select ;
use Carp ;

package NetClient ;
our $verbose = 0 ;

sub new
{
	my ( $classname , $port , $server , $timeout , $prompt ) = @_ ;
	$prompt ||= "\n" ;
	my $s = newSocket( $port , $server , $timeout ) ;
	return undef if !defined($s) ;
	return bless {
		m_prompt => $prompt ,
		m_port => $port ,
		m_server => $server ,
		m_timeout => $timeout ,
		m_s => $s ,
	} , $classname ;
}

sub newSocket
{
	my ( $port , $server , $connection_timeout ) = @_ ;
	return new IO::Socket(
		Domain => IO::Socket::AF_INET ,
		Type => IO::Socket::SOCK_STREAM ,
		Proto => 'tcp' ,
		Blocking => 0 , # but blocking connect
		Timeout => $connection_timeout ,
		PeerPort => $port ,
		PeerHost => $server ,
	) ;
}

sub send
{
	my ( $this , $tx ) = @_ ;
	if( !$this->{m_s}->connected() )
	{
		die "send: socket send error: $$this{m_server} $$this{m_port}: not connected" ;
	}
	$SIG{'PIPE'} = 'IGNORE' ;
	my $nsent = $this->{m_s}->send( $tx ) ;
	if( !defined($nsent) or $nsent != length($tx) )
	{
		die "send: socket send error: $$this{m_server} $$this{m_port}" ;
	}
}

sub read
{
	my ( $this , $prompt , $timeout ) = @_ ;
	if( !$this->{m_s}->connected() )
	{
		die "recv: socket receive error: $$this{m_server} $$this{m_port}: not connected" ;
	}
	$prompt = $this->{m_prompt} if !defined($prompt) ;
	$timeout = $this->{m_timeout} if !defined($timeout) ;
	$timeout = 99999 if $timeout < 0 ;

	my $loop = new IO::Select or die ;
	$loop->add( $this->{m_s} ) ;

	my $buffer ;
	my $t = time() ;
	my $t_end = $t + $timeout ;
	while( $t <= $t_end )
	{
		$!= 0 ;
		if( $loop->can_read($t_end-$t) )
		{
			my $length = 1 ; # one at a time so we dont read beyond the match
			my $data ;
			$this->{m_s}->recv( $data , $length ) ;
			return undef if !(defined($data) && $length) ; # disconnected
			$buffer .= $data ;
			_log( $buffer , $prompt ) if $verbose ;
			if( $buffer =~ m/$prompt/ )
			{
				# (used to strip $prompt from end of $buffer here)
				return $buffer ;
			}
		}
		return undef if $! != 0 ; # select error
		$t = time() ;
	}
	return undef ; # timeout
}

sub cmd
{
	my ( $this , $tx , $prompt , $timeout ) = @_ ;
	$prompt = $this->{m_prompt} if !defined($prompt) ;
	$timeout = $this->{m_timeout} if !defined($timeout) ;
	NetClient::send( $this , "$tx\r\n" ) ;
	return NetClient::read( $this , $prompt , $timeout ) ;
}

sub close
{
	my ( $this ) = @_ ;
	$this->{m_s}->close() ;
	$this->{m_s} = undef ;
}

sub _log
{
	my ( $buffer , $prompt ) = @_ ;
	my $m = ( $buffer =~ m/$prompt/ ) ? " **" : "" ;
	$buffer = $buffer =~ s/\r/\\r/gr =~ s/\n/\\n/gr ;
	$prompt = $prompt =~ s/\r/\\r/gr =~ s/\n/\\n/gr ;
	print "NetClient::read: [$buffer] [$prompt]$m\n" ;
}

1 ;
