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
# Filter.pm
#
# A function to create a filter script.
#
# Synopsis:
#
#	use Filter ;
#	Filter::create( $path ,
#		{
#			edit => 1 , # add a snippet to edit the envelope-from
#		} ,
#		{
#			unix => [
#				"sleep 1" ,
#				"exit 0" ,
#			] ,
#			win32 => [
#				'@echo off' ,
#			]
#		}
#	) ;
#

use strict ;
use FileHandle ;
use System ;

package Filter ;

sub create
{
	my ( $path , $opt , $lines_spec ) = @_ ;

	$opt ||= {} ;
	my $opt_edit = exists $opt->{edit} ;
	$opt->{os} = "" if ! exists $opt->{os} ;
	my $as_unix = ($opt->{os} eq "unix") ? 1 : ( ($opt->{os} =~ m/^win/) ? 0 : System::unix() ) ;

	System::log_( "creating [$path]" ) ;
	my $fh = new FileHandle( $path , "w" ) or die ;
	if( $as_unix )
	{
		print $fh "#!/bin/sh\n" ;
		print $fh 'content="$1"' , "\n" ;
		print $fh 'envelope="$2"' , "\n" ;
		if( $opt_edit )
		{
			# (sed -i is not quite portable enough)
			print $fh 'umask 0117' , "\n" ;
			print $fh 'set -e' , "\n" ;
			print $fh 'sed -E -e \'s/^(X-MailRelay-From:) (.*)(.)$/\1 FROM-EDIT\3/\' "$envelope" > "$envelope.edit" ' , "\n" ;
			print $fh 'mv -f "$envelope.edit" "$envelope"' , "\n" ;
		}
		for my $line ( @{$lines_spec->{unix}} )
		{
			print $fh $line , "\n" ;
		}
		$fh->close() or die ;
		system( "chmod +x \"$path\"" ) == 0 or die ;
	}
	else
	{
		print $fh 'var content = WScript.Arguments(0) ;' , "\n" ;
		print $fh 'var envelope = WScript.Arguments(1) ;' , "\n" ;
		if( $opt_edit )
		{
			print $fh '{' , "\n" ;
			print $fh '  var fs = WScript.CreateObject( "Scripting.FileSystemObject" ) ;' , "\n" ;
			print $fh '  var ts = fs.OpenTextFile( envelope , 1 , false ) ;' , "\n" ;
			print $fh '  var text = ts.ReadAll() ;' , "\n" ;
			print $fh '  ts.Close() ;' , "\n" ;
			print $fh '  var re = new RegExp( "^X-MailRelay-From: (\\\\S*)" , "m" ) ;' , "\n" ;
			print $fh '  text = text.replace( re , "X-MailRelay-From: FROM-EDIT" ) ;' , "\n" ;
			print $fh '  var ts = fs.OpenTextFile( envelope , 2 , false ) ;' , "\n" ;
			print $fh '  ts.Write( text ) ;' , "\n" ;
			print $fh '  ts.Close() ;' , "\n" ;
			print $fh '}' , "\n" ;
		}
		for my $line ( @{$lines_spec->{win32}} )
		{
			print $fh $line , "\n" ;
		}
		$fh->close() or die ;
	}
}

1 ;
