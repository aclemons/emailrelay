#!/usr/bin/env perl
#
# Copyright (C) 2001-2021 Graeme Walker <graeme_walker@users.sourceforge.net>
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
use Getopt::Std ;

# parse and mostly ignore sendmail command-line options
my $usage = "usage: emailrelay-sendmail [-intUv] [-BbCdFhNOopqRrVX <arg>] [-f <from>] <to> [<to> ...]" ;
my %opt = () ;
getopts( 'B:b:C:d:F:f:h:iN:nO:o:p:q:R:r:tUV:vX:' , \%opt ) or die "$usage\n" ;

# submit into the emailrelay spool directory
my $from = defined($opt{f}) ? $opt{f} : $ENV{USER} ;
my $exe = "/usr/sbin/emailrelay-submit" ;
my @args = ( $exe , "-f" , $from , @ARGV ) ; # also consider using "--copy" and "--content-date"
exec { $exe } @args ;
