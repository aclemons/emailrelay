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
# emailrelay-dkim-signer.pl
#
# An example E-MailRelay filter script for DKIM signing.
#
# To generate a key pair:
#   $ openssl genrsa -out example.com.pk 2048
#   $ openssl rsa -in example.com.pk -pubout -out example.com.pub
#
# Serve up the public key via DNS, eg:
#   $ grep -v PUBLIC example.com.pub | tr -d '\n'
#   upload default._domainkey TXT "p=Q8AMIIB...."
#   $ nslookup -query=TXT default._domainkey.example.com
#   text = "p=Q8AMIIB...."
#
# Test with spamassassin:
#   $ c=`emailrelay-submit -v -s \`pwd\` -C DQo= -C aGVsbG8sIHdvcmxkIQ== -d -F -t -f me@example.com you@example.com`
#   $ emailrelay-dkim-signer.pl $c
#   $ spamassassin --debug=dkim --test-mode < $c
#
# Requires debian package 'libmail-dkim-perl'.
#

use strict ;
use FileHandle ;
use File::Copy ;
use Mail::DKIM::Signer ;
use Mail::DKIM::TextWrap ;

$SIG{__DIE__} = sub { (my $e = join(" ",@_)) =~ s/\n/ /g ; print "<<error: $e>>\n" ; exit 99 } ;

my $content = $ARGV[0] or die "usage error\n" ;
my $fh = new FileHandle( $content ) or die "cannot open content file\n" ;

my $dkim = new Mail::DKIM::Signer(
	Algorithm => 'rsa-sha1' ,
	Method => 'relaxed' ,
	Domain => 'example.com' ,
	Selector => 'default' , # => default._domainkey.example.com
	KeyFile => '/etc/dkim/private/example.com.pk' ,
	Headers => '' , # 'x-header:x-header2'
);

$dkim->load( $fh ) ; # includes CLOSE()
$fh->close() or die ;

my $signature = $dkim->signature->as_string() ;

$fh = new FileHandle( $content.".tmp" , "w" ) or die ;
print $fh $signature , "\r\n" ;
$fh->flush() ;
File::Copy::copy( $content , $fh ) or die ;
$fh->close() or die ;
File::Copy::move( $content.".tmp" , $content ) or die ;

exit 0 ;
