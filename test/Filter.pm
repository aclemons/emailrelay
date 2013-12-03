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
# Filter.pm
#
# Filter->create() creates a filter script.
#

use strict ;
use FileHandle ;

package Filter ;

sub create
{
	my ( $path , $lines_spec ) = @_ ;
	my $file = new FileHandle( "> $path" ) ;
	if( System::unix() )
	{
		print $file "#!/bin/sh\n" ;
		for my $line ( @{$lines_spec->{unix}} )
		{
			print $file $line , "\n" ;
		}
		system( "chmod +x $path" ) ;
	}
	else
	{
		for my $line ( @{$lines_spec->{win32}} )
		{
			print $file $line , "\n" ;
		}
	}
}

1 ;

