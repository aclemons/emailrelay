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

# run emailrelay-submit
my @cmd = ( $exe ) ;
push @cmd , ("-f",$from) if $from ;
push @cmd , ( "--content-date" , "--content-message-id=local" , "--" ) ; # maybe also "--copy"`
push @cmd , @ARGV ; # 'to' addresses
exec { $exe } @cmd ;

