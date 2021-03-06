#!/usr/bin/env perl
#
# Copyright (C) 2001-2020 Graeme Walker <graeme_walker@users.sourceforge.net>
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
		@headers[-1] .= "\r\n$line" ;
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

