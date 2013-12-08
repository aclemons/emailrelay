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
# AdminClient.pm
#
# A network client to drive the admin interface.
#

use strict ;
use FileHandle ;
use Net::Telnet ;

package AdminClient ;

sub new
{
	my ( $classname , $port , $server ) = @_ ;

	$server = defined($server) ? $server : "localhost" ;
	$port = defined($port) ? $port : 10026 ;

	my $t = new Net::Telnet( Timeout=>3 , Prompt=>"/E-MailRelay> /" ) ;
	$t->max_buffer_length(1000000*10) ;

	my %me = (
		m_port => $port ,
		m_server => $server ,
		m_t => $t ,
	) ;
	return bless \%me , $classname ;
}

sub open
{
	my ( $this ) = @_ ;
	return $this->{'m_t'}->open( Host=>$this->server() , Port=>$this->port() ) ;
}

sub port { return shift->{m_port} }
sub server { return shift->{m_server} }
sub doHelp { shift->{'m_t'}->cmd("help") }
sub doTerminate { shift->{'m_t'}->print("terminate") }
sub doFlush { shift->{'m_t'}->print("flush") }
sub getline { return shift->{'m_t'}->getline() }

sub doList 
{ 
	my ( $this ) = @_ ;
	$this->{'m_t'}->buffer_empty() ; # just in case
	my @output = $this->{'m_t'}->cmd("list") ;
	my @result = () ;
	for my $s ( @output )
	{
		chomp $s ;
		if( $s ne "<none>" )
		{
			push @result , $s ;
		}
	}
	return @result ;
}

1 ;

