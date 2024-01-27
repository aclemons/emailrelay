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
# emailrelay-sendmail.pl
#
# A sendmail/emailrelay shim. Typically installed as /usr/sbin/sendmail or
# /usr/lib/sendmail.
#

use strict ;
use FileHandle ;
use Getopt::Std ;

my $exe = "/usr/sbin/emailrelay-submit" ;
my $usage = "usage: emailrelay-sendmail [-intUv] [-ABbCDdFfGhiLNnOopqRrtUVvX <arg>] [-f <from>] <to> [<to> ...]" ;

# parse and mostly ignore sendmail command-line options
my %opt = () ;
$Getopt::Std::STANDARD_HELP_VERSION = 1 ;
sub HELP_MESSAGE() { print "$usage\n" }
sub VERSION_MESSAGE() {}
getopts( 'A:B:b:C:D:d:F:f:Gh:iL:N:nO:o:p:q:R:r:tUV:vX:' , \%opt ) or die "$usage\n" ;
my $from = defined($opt{f}) ? $opt{f} : $ENV{USER} ;
my @submit_args = ( "--content-date" , @ARGV ) ; # also consider adding "--copy" here

# if "-t" read envelope 'to' addresses from content headers
if( $opt{t} )
{
	# read content headers
	my %h = () ;
	my @h = () ;
	my $k = "" ;
	while(<STDIN>)
	{
		chomp( my $line = $_ ) ;
		$line =~ s/\r$// ;
		last if ( $line eq "" ) ;
        my ( $a , $b , $c , $d ) = ( $line =~ m/^(\S*):\s*(.*)|^(\s)(.*)/ ) ;
        if( $a ) { $k = lc($a) ; $h{$k} = $b }
        if( $k && $d ) { $h{$k} .= "$c$d" } # folding
		push @h , $line unless ( lc($k) eq 'bcc' ) ; # remove Bcc
	}

	# extract 'to' addresses
	my @to = () ;
	push @to , split(" ",$h{to}) if exists($h{to}) ;
	push @to , split(" ",$h{cc}) if exists($h{cc}) ;
	push @to , split(" ",$h{bcc}) if exists($h{bcc}) ;

	# write headers and copy body into temp file
	my $tmp = "/tmp/emailrelay-sendmail.$$.tmp" ;
	my $fh = new FileHandle( $tmp , "w" ) or die ;
	map { print $fh $_ , "\r\n" } @h ;
	print $fh "\r\n" ;
	while(<STDIN>) { print $fh $_ }
	$fh->close() or die ;

	# make stdin read from the temp file
	open STDIN , '<' , $tmp or die ;
	unlink $tmp or die ;

	# run emailrelay-submit
	my @args = ( $exe , "-f" , $from , @to ) ;
	exec { $exe } @args ;
}
else
{
	# run emailrelay-submit
	my @args = ( $exe , "-f" , $from , @submit_args ) ;
	exec { $exe } @args ;
}
