#!/usr/bin/env perl
#
# Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
# 
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.
# ===
#
# emailrelay-set-from.pl
#
# An example E-MailRelay "--filter" script that edits the content originator
# fields (ie. From, Sender and Reply-To) to a fixed value.
#
# Also consider setting the envelope-from field by editing the envelope
# file.
#
# See also: emailrelay-set-from.js, RFC-2822
#

use strict ;
use FileHandle ;
$SIG{__DIE__} = sub { (my $e = join(" ",@_)) =~ s/\n/ /g ; print "<<error: $e>>\n" ; exit 99 } ;

# originator fields (RFC-2822 3.6.2)
my $new_from = 'noreply@example.com' ;
my $new_sender = '' ;
my $new_reply_to = $new_from ;

my $content = $ARGV[0] or die "usage error\n" ;

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
