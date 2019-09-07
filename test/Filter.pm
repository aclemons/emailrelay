#!/usr/bin/perl
#
# Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#	Filter::create( $path , {} , { unix=>["sleep 1","exit 0"] , win32=>['@echo off'] } ) ;
#

use strict ;
use FileHandle ;
use File::Basename ;
use lib File::Basename::dirname($0) ;
use System ;

package Filter ;

sub create
{
	my ( $path , $opt , $lines_spec ) = @_ ;

	$opt ||= {} ;
	my $opt_client = exists $opt->{client} ;
	my $opt_edit = exists $opt->{edit} ;
	$opt->{os} = "" if ! exists $opt->{os} ;
	my $as_unix = ($opt->{os} eq "unix") ? 1 : ( ($opt->{os} =~ m/^win/) ? 0 : System::unix() ) ;

	my $file = new FileHandle( $path , "w" ) ;
	if( $as_unix )
	{
		print $file "#!/bin/sh\n" ;
		print $file 'content="$1"' , "\n" ;
		print $file 'envelope="`echo $content | sed \'s/content$/envelope.'.($opt_client?"busy":"new").'/\'`"' , "\n" ;
		if( $opt_edit )
		{
			# (sed -i is not quite portable enough)
			print $file 'umask 0117' , "\n" ;
			print $file 'set -e' , "\n" ;
			print $file 'sed -E -e \'s/^(X-MailRelay-From:) (.*)(.)$/\1 FROM-EDIT\3/\' "$envelope" > "$envelope.edit" ' , "\n" ;
			print $file 'mv -f "$envelope.edit" "$envelope"' , "\n" ;
		}
		for my $line ( @{$lines_spec->{unix}} )
		{
			print $file $line , "\n" ;
		}
		system( "chmod +x $path" ) ;
	}
	else
	{
		print $file 'var content = WScript.Arguments(0) ;' , "\n" ;
		print $file 'var envelope = WScript.Arguments(1) ;' , "\n" ;
		if( $opt_edit )
		{
			print $file '{' , "\n" ;
			print $file '  var fs = WScript.CreateObject( "Scripting.FileSystemObject" ) ;' , "\n" ;
			print $file '  var ts = fs.OpenTextFile( envelope , 1 , false ) ;' , "\n" ;
			print $file '  var text = ts.ReadAll() ;' , "\n" ;
			print $file '  ts.Close() ;' , "\n" ;
			print $file '  var re = new RegExp( "^X-MailRelay-From: (\\\\S*)" , "m" ) ;' , "\n" ;
			print $file '  text = text.replace( re , "X-MailRelay-From: FROM-EDIT" ) ;' , "\n" ;
			print $file '  var ts = fs.OpenTextFile( envelope , 2 , false ) ;' , "\n" ;
			print $file '  ts.Write( text ) ;' , "\n" ;
			print $file '  ts.Close() ;' , "\n" ;
			print $file '}' , "\n" ;
		}
		for my $line ( @{$lines_spec->{win32}} )
		{
			print $file $line , "\n" ;
		}
	}
}

1 ;

