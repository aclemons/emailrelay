#!/usr/bin/env perl
#
# Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
# 
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.
# ===
#
# emailrelay-rot13.pl
#
# An example E-MailRelay "--filter" script that does rot-13 masking.
#

use strict ;
use FileHandle ;
$SIG{__DIE__} = sub { (my $e = join(" ",@_)) =~ s/\n/ /g ; print "<<error: $e>>\n" ; exit 99 } ;

my $content = $ARGV[0] ;
my $content_tmp = "$content.tmp" ;
my $fh_in = new FileHandle( $content , "r" ) or die "cannot open content file [$content]: $!\n" ;
my $fh_out = new FileHandle( "$content_tmp" , "w" ) or die "cannot open temporary file [$content_tmp]: $!\n" ;
my $boundary = "-----emailrelay-rot13-$$" ;
my $in_header = 1 ;
my @headers = () ;
while(<$fh_in>)
{
	chomp( my $line = $_ ) ;
	$line =~ s/\r$// ;

	if( $in_header && ( $line =~ m/^\s/ ) && scalar(@headers) ) # folding
	{
		$headers[-1] .= "\r\n$line" ;
	}
	elsif( $in_header && ( $line =~ m/^$/ ) )
	{
		$in_header = 0 ;
		for my $h ( @headers )
		{
			if( $h =~ m/^(subject|to|from):/i )
			{
				print $fh_out $h , "\r\n" ;
			}
		}
		print $fh_out "Content-Type: multipart/mixed; boundary=\"$boundary\"\r\n" ;
		print $fh_out "\r\n" ;
		print $fh_out "\r\n" ;
		print $fh_out "--$boundary\r\n" ;
		print $fh_out "Content-Type: text/plain; charset=us-ascii\r\n" ;
		print $fh_out "\r\n" ;
		print $fh_out "The original message has been masked...\r\n" ;
		print $fh_out "\r\n" ;
		print $fh_out "--$boundary\r\n" ;
		print $fh_out "Content-Type: text/plain\r\n" ;
		print $fh_out "Content-Transfer-Encoding: 8bit\r\n" ;
		print $fh_out "Content-Description: masked message\r\n" ;
		print $fh_out "\r\n" ;
		print $fh_out join( "\r\n" , map { rot13($_) } (@headers,"") ) ;
	}
	elsif( $in_header )
	{
		push @headers , $line ;
	}
	else
	{
		print $fh_out rot13($line) , "\r\n" ;
	}
}
print $fh_out "--$boundary--\r\n" ;
print $fh_out "\r\n" ;

$fh_in->close() or die ;
$fh_out->close() or die "cannot write new file [$content_tmp]: $!\n" ;
unlink( $content ) or die "cannot delete original file [$content]: $!\n" ;
rename( $content_tmp , $content ) or die "cannot rename [$content_tmp]: $!\n" ;
exit( 0 ) ;

sub rot13
{
	my ( $s ) = @_ ;
	$s =~ tr/[a-m][n-z][A-M][N-Z]/[n-z][a-m][N-Z][A-M]/ ;
	return $s ;
}

