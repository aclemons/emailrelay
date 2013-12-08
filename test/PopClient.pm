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
# PopClient.pm
#
# A network client for driving the pop interface.
#

use strict ;
use FileHandle ;
use Net::Telnet ;

package PopClient ;

sub new
{
	my ( $classname , $port , $server ) = @_ ;

	$server = defined($server) ? $server : "localhost" ;
	$port = defined($port) ? $port : 10110 ;
	my $timeout = 10 ;

	my $t = new Net::Telnet( Timeout=>$timeout , Prompt=>'/\+OK[^\r\n]*/' ) ;
	$t->binmode(0) ; # convert to '\r\n' on output
	#$t->max_buffer_length(1000000*10) ; # allow for long message listings

	my %me = (
		m_port => $port ,
		m_server => $server ,
		m_t => $t ,
	) ;
	return bless \%me , $classname ;
}

sub open
{
	my ( $this , $wait ) = @_ ;
	$wait = defined($wait) ? $wait : 1 ;
	my $t = $this->t() ;
	my $ok = $t->open( Host=>$this->server() , Port=>$this->port() ) ;
	my ($s1,$s2) = $t->waitfor( '/\+OK[^\r\n]*/' ) if $wait ;
	return $ok ;
}

sub port { return shift->{'m_port'} }
sub server { return shift->{'m_server'} }
sub t { return shift->{'m_t'} }

sub login
{
	my ( $this , $name , $pwd ) = @_ ;
	my $t = $this->t() ;
	$t->cmd( "user $name" ) ;
	$t->cmd( "pass $pwd" ) ; # allow for slow directory locking
	return 1 ;
}

sub list
{
	my ( $this , $read_slowly , $timeout ) = @_ ;
	my $t = $this->t() ;
	$t->cmd( "list" ) ;
	my $result = "" ;
	if( $read_slowly )
	{
		while(1)
		{
			my ($s1,$s2) = $t->waitfor( Match => '/[ \.]/' , Timeout => $timeout ) ;
			if( $s2 eq " " )
			{
				#print STDERR "++++ [$s1]\n" ;
				System::sleep_cs( 1 ) ;
				$result .= $s1 ;
				$result .= $s2 ;
			}
			else
			{
				$result .= $s1 ;
				last ;
			}
		}
	}
	else
	{
		my ($s1,$s2) = $t->waitfor( Match => '/\./' , Timeout => $timeout ) ;
		#print STDERR "++++ [$s1]\n" ;
		$result = $s1 ;
	}
	my @list = split("\n",$result) ;
	if( scalar(@list) && $list[0] eq "" )
	{
		shift @list ;
	}
	return @list ;
}

sub disconnect
{
	my ( $this ) = @_ ;
	my $t = $this->t() ;
	$t->close() ;
}

1 ;

