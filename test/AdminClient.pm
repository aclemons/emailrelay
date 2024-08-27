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
# AdminClient.pm
#
# A network client to drive the admin interface.
#
# Synopsis:
#
#	my $ac = new AdminClient(10026,"localhost") ;
#	$ac->open() or die ; # first
#	$rsp = $ac->doHelp() ;
#	$ac->doTerminate() ;
#	$ac->doFlush() ;
#	$line = $ac->getline() ; # last
#

use strict ;
use FileHandle ;
use NetClient ;
use System ;

package AdminClient ;

sub new
{
	my ( $classname , $port , $server ) = @_ ;

	$port ||= 10026 ;
	$server ||= $System::localhost ;

	my %me = (
		m_port => $port ,
		m_server => $server ,
		m_prompt => qr/E-MailRelay> / ,
		m_timeout => 3 ,
		m_nc => undef ,
	) ;
	return bless \%me , $classname ;
}

sub port { return shift->{m_port} }
sub server { return shift->{m_server} }
sub doHelp { return $_[0]->{m_nc}->cmd( "help" ) }
sub doTerminate { $_[0]->{m_nc}->send( "terminate\r\n" ) }
sub doFlush { $_[0]->{m_nc}->send( "flush\r\n") }
sub doForward { $_[0]->{m_nc}->cmd( "forward") }

sub open
{
	my ( $this ) = @_ ;
	$this->{m_nc} = new NetClient( $this->{m_port} , $this->{m_server} , $this->{m_timeout} , $this->{m_prompt} ) ;
	return defined($this->{m_nc}) ;
}

sub getline
{
	my ( $this , $timeout ) = @_ ;
	my $line = $this->{m_nc}->read( qr/\n/ , $timeout ) ;
	$line = "" if !defined($line) ;
	$line =~ s/\r//g ;
	$line =~ s/\n$// ;
	return $line ;
}

1 ;
