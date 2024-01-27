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
# emailrelay-bcc-check.pl
#
# An example E-MailRelay "--filter" script that rejects e-mail messages that
# have a non-empty "Bcc:" recipient list unless the "Bcc:" recipient list
# contains a single addressee that matches the envelope recipient. This has
# the effect of checking that a submitting user agent is behaving correctly
# as per RFC-5322 3.6.3.
#
# Bcc handling is generally only a concern for e-mail user agent programs
# and not relays and proxies. User agents should normally submit a message
# separately for each Bcc recipient with either no "Bcc:" field or with the
# "Bcc:" field containing that one recipient.
#
# Note that correct parsing of content files is beyond the scope of a simple
# example script like this, and incorrect Bcc handling can have a serious
# privacy implications.
#

use strict ;
use FileHandle ;
$SIG{__DIE__} = sub { (my $e = join(" ",@_)) =~ s/\n/ /g ; print "<<error: $e>>\n" ; exit 99 } ;

my $content = $ARGV[0] or die "usage error\n" ;
my $verbose = 1 ;

# read the bcc list from the content file
my %c = headers( $content ) ;
my @bcc = split( /,/ , $c{Bcc} ) ; # todo -- allow for commas in quoted strings
map { print "BCC=[$_]\n" } @bcc if $verbose ;

# allow if there is no bcc list or it's empty
exit 0 if scalar(@bcc) == 0 ;
my $bcc = $bcc[0] =~ s/^\s*"?//r =~ s/"?\s*$//r ;
exit 0 if( $bcc =~ m/^\s*$/ ) ;

# read the recipient list from the envelope file
my @rcp = read_fields( find_envelope($content) , qr/^X-MailRelay-To-Remote:\s*(.*)/ ) ;
map { print "RECIPIENT=[$_]\n" } @rcp if $verbose ;

# allow if there are no (remote) recipients at all
exit 0 if scalar(@rcp) == 0 ;
my $rcp = $rcp[0] ;

# allow if one recipient matching one bcc
exit 0 if( scalar(@rcp) == 1 && scalar(@bcc) == 1 && $bcc =~ m/\Q$rcp\E/ ) ;

# deny otherwise
print "<<bcc error>>\n" ;
exit 1 ;

sub headers
{
	my ( $file ) = @_ ;
	my $fh = open_file( $file ) ;
	my %h = read_headers( $fh ) ;
	$fh->close() or die ;
	return %h ;
}

sub read_headers
{
	my ( $fh ) = @_ ;
	my %h = () ;
	my $k ;
	while(<$fh>)
	{
		chomp( my $line = $_ ) ;
		$line =~ s/\r$// ;
		last if ( $line eq "" ) ;
		my ( $a , $b , $c , $d ) = ( $line =~ m/^(\S*):\s*(.*)|^(\s)(.*)/ ) ;
		if( $a ) { $h{$a} = $b ; $k = $a }
		if( $k && $d ) { $h{$k} .= "$c$d" } # folding
	}
	return %h ;
}

sub open_file
{
	my ( $file ) = @_ ;
	my $fh = new FileHandle( $file , "r" ) or die "cannot open [$file]: $!\n" ;
	return $fh ;
}

sub envelope
{
	my ( $content , $ext ) = @_ ;
	$ext = "" if !defined($ext) ;
	$content =~ m/\.content$/ or die "invalid content filename [$content]\n" ;
	( my $envelope = $content ) =~ s/\.content$/.envelope/ ;
	$envelope .= $ext ;
	return $envelope ;
}

sub find_envelope
{
	my ( $content ) = @_ ;
	map { return $_ if -f $_ } map { envelope($content,$_) } ( "" , ".new" , ".busy" ) ;
	die "no envelope for [$content]\n" ;
}

sub read_fields
{
	my ( $file , $re ) = @_ ;
	return map { s/$re/$1/ ; $_ } grep { m/$re/ } read_all( $file ) ;
}

sub read_all
{
	my ( $file ) = @_ ;
	my $fh = open_file( $file ) ;
	my @lines = () ;
	while(<$fh>)
	{
		chomp( my $line = $_ ) ;
		$line =~ s/\r$// ;
		push @lines , $line ;
	}
	$fh->close() or die ;
	return @lines ;
}

