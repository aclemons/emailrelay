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
# Filter.pm
#
# A function to create a filter script.
#
# Synopsis:
#
#	use Filter ;
#	Filter::create( $path ,
#		{
#			edit => {From=>'EDIT-FROM'} , # add a snippet to edit the envelope-from
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
	# Creates a filter script with code to edit an envelope file
	# and/or lines of code as passed in.
	#
	# Eg: Filter::create( "filter" , {os=>unix,edit=>{From=>'FROM',To-Remote=>'TO'}} ) ;
	#
	my ( $path , $opt , $lines_spec ) = @_ ;

	$opt ||= {} ;
	$opt->{os} = "" if ! exists $opt->{os} ;

	# backwards compatiblity for "{edit=>1}"
	if( exists($opt->{edit}) && !ref($opt->{edit}) )
	{
		$opt->{edit} = {From=>'FROM-EDIT'} ;
	}

	System::log_( "creating [$path]" ) ;
	my $fh = new FileHandle( $path , "w" ) or die ;
	if( ($opt->{os} eq "unix") ? 1 : ( ($opt->{os} =~ m/^win/) ? 0 : System::unix() ) )
	{
		print $fh "#!/bin/sh\n" ;
		print $fh 'content="$1"' , "\n" ;
		print $fh 'envelope="$2"' , "\n" ;
		if( defined($opt->{edit}) )
		{
			print $fh 'umask 0117' , "\n" ;
			print $fh 'set -e' , "\n" ;
			print $fh 'sed -E' ;
			for my $field ( keys %{$opt->{edit}} )
			{
				my $value = $opt->{edit}->{$field} ;
				print $fh ' -e \'s/^(X-MailRelay-'.$field.':) ([^\r]*)(.*)$/\1 '.$value.'\3/\''
			}
			print $fh ' "$envelope" > "$envelope.edit"' , "\n" ;
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
		if( defined($opt->{edit}) )
		{
			print $fh '{' , "\n" ;
			print $fh '  var fs = WScript.CreateObject( "Scripting.FileSystemObject" ) ;' , "\n" ;
			print $fh '  var ts = fs.OpenTextFile( envelope , 1 , false ) ;' , "\n" ;
			print $fh '  var text = ts.ReadAll() ;' , "\n" ;
			print $fh '  ts.Close() ;' , "\n" ;
			for my $field ( keys %{$opt->{edit}} )
			{
				my $value = $opt->{edit}->{$field} ;
				print $fh '  var re = new RegExp( "^X-MailRelay-'.$field.': (\\\\S*)" , "m" ) ;' , "\n" ;
				print $fh '  text = text.replace( re , "X-MailRelay-'.$field.': '.$value.'" ) ;' , "\n" ;
			}
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
