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
# Port.pm
#
# A module for checking open ports.
#

use strict ;
use FileHandle ;

package Port ;

sub list
{
	my $netstat = ( System::mac() || System::bsd() ) ? "netstat -an -f inet" : "netstat -ant" ;
	my $f = new FileHandle( "$netstat |" ) ;
	my $nr = 1 ;
	my @ports = () ;
	for( ; <$f> ; $nr++ )
	{
		my $line = $_ ;
		if( $nr > 2 )
		{
			my @line_part = split( /\s+/ , $line ) ;
			my $address = $line_part[3] ;
			my ( $port ) = ( $address =~ m/([0-9]+)$/ ) ;
			if( defined($line_part[5]) && $line_part[5] eq "LISTEN" )
			{
				push @ports , $port ;
			}
		}
	}
	return @ports ;
}

sub isOpen
{
	my ( $port , @port_list ) = @_ ;
	for my $p ( @port_list )
	{
		return 1 if( $p == $port ) ;
	}
	return 0 ;
}

sub isFree
{
	my ( $port , @port_list ) = @_ ;
	return !isOpen($port,@port_list) ;
}

1 ;

