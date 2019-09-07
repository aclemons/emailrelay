#!/usr/bin/env perl
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
# emailrelay-set-from.pl
#
# An example E-MailRelay "--filter" script that edits the content originator
# fields (ie. From, Sender and Reply-To) to a fixed value.
#
# See also: RFC-2822
#

use strict ;
use FileHandle ;
$SIG{__DIE__} = sub { (my $e = join(" ",@_)) =~ s/\n/ /g ; print "<<error: $e>>\n" ; exit 99 } ;

# originator fields (RFC-2822 3.6.2)
my $new_from = 'noreply@example.com' ;
my $new_sender = '' ;
my $new_reply_to = $new_from ;

my $content = @ARGV[0] or die "usage error\n" ;

my $in = new FileHandle( $content , "r" ) or die ;
my $out = new FileHandle( "$content.tmp" , "w" ) or die ;
my $in_body = undef ;
my $in_edit = undef ;
while(<$in>)
{
	if( $in_body )
	{
		print $out $_ ;
	}
	else
	{
		chomp( my $line = $_ ) ;
		$line =~ s/\r$// ;

		$in_body = 1 if ( $line eq "" ) ;
		my $is_from = ( $line =~ m/^From:/i ) ;
		my $is_sender = ( $line =~ m/^Sender:/i ) ;
		my $is_reply_to = ( $line =~ m/^Reply-To:/i ) ;

		if( $in_body )
		{
			print $out "\r\n" ;
		}
		elsif( $is_from && defined($new_from) )
		{
			$in_edit = 1 ;
			print $out "From: $new_from\r\n" ;
		}
		elsif( $is_sender && defined($new_sender) )
		{
			$in_edit = 1 ;
			print $out "Sender: $new_sender\r\n" unless $new_sender eq "" ;
		}
		elsif( $is_reply_to && defined($new_reply_to) )
		{
			$in_edit = 1 ;
			print $out "Reply-To: $new_reply_to\r\n" ;
		}
		elsif( $in_edit && $line =~ m/^[ \t]/ ) # original header was folded
		{
		}
		else
		{
			$in_edit = undef ;
			print $out $line , "\r\n" ;
		}
	}
}
$out->close() or die ;
rename( "$content.tmp" , $content ) or die ;
exit 0 ;
