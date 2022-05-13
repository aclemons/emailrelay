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
		m_s => undef ,
	) ;
	return bless \%me , $classname ;
}

sub port { return shift->{m_port} }
sub server { return shift->{m_server} }
sub doHelp { return NetClient::cmd( $_[0] , "help" ) }
sub doTerminate { NetClient::send( $_[0] , "terminate\r\n" ) }
sub doFlush { NetClient::send( $_[0] , "flush\r\n") }

sub open
{
	my ( $this ) = @_ ;
	$this->{m_s} = NetClient::newSocket( $this ) ;
	return !! $this->{m_s} ;
}

sub getline
{
	my ( $this , $timeout ) = @_ ;
	$timeout ||= $this->{m_timeout} ;
	my $line = NetClient::read( $this , qr/\n/ , $timeout ) ;
	$line = "" if !defined($line) ;
	$line =~ s/\r//g ;
	$line =~ s/\n$// ;
	return $line ;
}

1 ;
