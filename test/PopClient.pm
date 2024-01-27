#!/usr/bin/perl
#
# Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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
# Synopsis:
#
#	my $pc = new PopClient( 10101 , "localhost" ) ;
#	$pc->open() or die ;
#	$pc->login("me","secret") or die ;
#	my @list = $pc->list() ;
#	$pc->disconnect() ;
#

use strict ;
use FileHandle ;
use NetClient ;
use System ;

package PopClient ;

sub new
{
	my ( $classname , $port , $server ) = @_ ;

	$port ||= 10110 ;
	$server ||= $System::localhost ;

	my %me = (
		m_port => $port ,
		m_server => $server ,
		m_prompt => qr/\+OK[^\n]*\n/ ,
		m_timeout => 10 ,
		m_nc => undef ,
	) ;
	return bless \%me , $classname ;
}

sub port { return shift->{'m_port'} }
sub server { return shift->{'m_server'} }

sub open
{
	my ( $this , $wait ) = @_ ;
	$wait = defined($wait) ? $wait : 1 ;

	$this->{m_nc} = new NetClient( $this->{m_port} , $this->{m_server} , $this->{m_timeout} , $this->{m_prompt} ) ;
	$this->{m_nc}->read( undef , -1 ) if $wait ;
	return 1 ;
}

sub login
{
	my ( $this , $name , $pwd ) = @_ ;
	$this->{m_nc}->cmd( "user $name" ) ;
	$this->{m_nc}->cmd( "pass $pwd" ) ;
	return 1 ;
}

sub list
{
	my ( $this , $read_slowly__not_used ) = @_ ;
	my $s = $this->{m_nc}->cmd( "list" , qr/\.\r\n/ ) ;
	$s =~ s/^$$this{m_prompt}// ;
	$s =~ s/\.\r\n$// ;
	$s =~ s/\r//g ;
	return split( /\n/ , $s ) ;
}

sub disconnect
{
	my ( $this ) = @_ ;
	$this->{m_nc} = undef ;
}

1 ;
