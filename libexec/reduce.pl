#!/usr/bin/perl
#
# Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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
# reduce.pl
#
# Analyses a single source file for redundancy and
# outputs (--out) its conclusions, which can be used
# for a later edit (--in). Also supplies a "--sources"
# option to parse an automake makefile.
#
# See also "Reduce.pm" and "reduce".
#
# usage:
#     reduce.pl [--out <results-file> [--append]] [--] <src> <make-command> [<make-command> ...]
#     reduce.pl --in <results-file>
#     reduce.pl --undo <src-base-dir>
#     reduce.pl --sources <makefile-dir>
#

use strict ;
use File::Copy ;
use File::Basename ;
use File::Find ;
use lib File::Basename::dirname($0) ;
use FileHandle ;
use Getopt::Long ;
use Functions ;
use Reduce ;
use AutoMakeParser ;
use ConfigStatus ;

my %opt = () ;
my $usage = "reduce [--in=<results>] [--out=<results> [--append]] [--] <src> [<make-command> [<make-command> ...]]" ;
GetOptions( \%opt , "quiet" , "debug" , "undo=s" , "in=s" , "out=s" , "append" , "sources=s" ) or die "usage: $usage\n" ;

if( $opt{in} )
{
	my $reduce = new Reduce() ;
	$reduce->read( $opt{in} ) ;
	my @basenames = sort $reduce->basenames() ;
	print "reduce: files to edit: " , join(" ",@basenames) , "...\n" unless $opt{quiet} ;
	for my $basename ( @basenames )
	{
		my ( $src ) = glob( "../*/$basename" ) ;
		if( ! -f $src )
		{
			print "reduce: $basename: file not found from glob(\"../*/$basename\") :-<\n" ;
			next ;
		}
		my $tmp = "reduce-$basename.tmp" ;
		if( $reduce->edit( $src , $tmp ) )
		{
			File::Copy::copy( $tmp , $src ) or die ;
		}
		unlink( $tmp ) ;
	}
}
elsif( $opt{undo} )
{
	sub _undo_file
	{
		my $path = $File::Find::name ;
		if( $path =~ m/\.cpp$/ )
		{
			my $basename = File::Basename::basename( $path ) ;
			my $tmp = "reduce-undo-$basename.tmp" ;
			my $changed = 0 ;
			my $fh_in = new FileHandle( $path ) or die ;
			my $fh_out = new FileHandle( $tmp , "w" ) or die ;
			my $in_block = 0 ;
			while(<$fh_in>)
			{
				chomp( my $line = $_ ) ;
				if( $line =~ m/^#ifndef G_LIB_SMALL$/ )
				{
					$changed = 1 ;
					$in_block++ ;
				}
				elsif( $in_block && $line =~ m/^#endif/ )
				{
					$changed = 1 ;
					$in_block-- ;
				}
				else
				{
					print $fh_out $line , "\n" ;
				}
			}
			$fh_out->close() or die ;
			if( $changed )
			{
				File::Copy::copy( $tmp , $path ) or die ;
				print "reduce: [$path] undone\n" unless $opt{quiet} ;
			}
			else
			{
				print "reduce: [$path] nothing to do\n" unless $opt{quiet} ;
			}
			unlink( $tmp ) ;
		} ;
	}
	File::Find::find( {no_chdir=>1,wanted=>\&_undo_file} , $opt{undo} ) ;
}
elsif( $opt{sources} )
{
	my $makefile_dir = $opt{sources} ;
	die "no makefile" if ! -e "${makefile_dir}/Makefile.am" ;
	my $cs = new ConfigStatus() ;
	my $top = File::Basename::dirname( $cs->path() ) ;
	my %vars = ( $cs->vars() , top_srcdir => $top ) ;
	my $parser = new AutoMakeParser( "$makefile_dir/Makefile.am" , $cs->switchesref() , \%vars ) ;
	my $libname = "lib" . File::Basename::basename($makefile_dir) . ".a" ;
	my @sources = $parser->sources( $libname ) ;
	print join( " " , sort @sources ) , "\n" ;
}
elsif( scalar(@ARGV) >= 2 )
{
	my $arg_src = shift @ARGV ;
	my @arg_make_commands = @ARGV ;
	my $fh_out ;
	if( $opt{out} ) { $fh_out = new FileHandle( $opt{out} , $opt{append} ? "a" : "w" ) or die }
	my $reduce = new Reduce( \@arg_make_commands , {debug=>$opt{debug}} ) ;
	$reduce->check( $arg_src , !$opt{out} ) ;
	$reduce->emit( $fh_out ) if($fh_out) ;
}
else
{
	die "$usage\n" ;
}
exit( 0 ) ;
