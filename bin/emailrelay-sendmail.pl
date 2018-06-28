#!/usr/bin/env perl
#
# Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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
# emailrelay-sendmail.pl
#
# A sendmail/emailrelay shim. Typically installed as /usr/sbin/sendmail or /usr/lib/sendmail.
#
use strict ;
use Getopt::Std ;
use FileHandle ;

my $usage = "usage: emailrelay-sendmail [-intUv] [-BbCdFhNOopqRrVX <arg>] [-f <from>]" ;
my %opt = () ;
getopts( 'B:b:C:d:F:f:h:iN:nO:o:p:q:R:r:tUV:vX:' , \%opt ) or die "$usage\n" ;

my $from = defined($opt{f}) ? $opt{f} : $ENV{USER} ;
my @args = ( "-f" , $from ) ;
my $exe = "/usr/sbin/emailrelay-submit" ;
exec { $exe } @args ;
