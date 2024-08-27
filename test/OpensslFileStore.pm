#!/usr/bin/perl
#
# Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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
# OpensslFileStore.pm
#
# A package that simplifies the management of temporary certificate files.
#
# Synopsis:
#	use OpensslFileStore ;
#	$fs = new OpensslFileStore( "certs" , ".pem" ) ;
#	$path = $fs->catfile( "bob.key" , "bob.crt" ) ;
#	$path = $fs->tmpfile( "bob.csr" ) ;
#	$path = $fs->infile( "alice.key" ) ;
#	$path = $fs->outfile( "alice.crt" ) ;
#	$path = $fs->logfile() ;
#	$fs->cleanup( 'cat' , 'tmp' , 'out' , 'log' ) ;
#
# The implementation can be customised with respect to logging, file paths
# and file deletion, eg:
#	$OpensslFileStore::log_cat_fn = sub { my ($in,@out)=@_ ; print "creating [$in]\n" } ;
#	$OpensslFileStore::outpath_fn = sub { my ($dir,$fname)=@_ ; return "/tmp/$fname" } ;
#	$OpensslFileStore::inpath_fn = sub { my ($dir,$fname)=@_ ; return "/tmp/$fname" } ;
#	$OpensslFileStore::unlink_fn = sub { my ($path)=@_ ; unlink $path } ;
#
# Or perhaps:
#	use System ;
#	$OpensslFileStore::log_cat_fn = sub { System::log_("creating [$_[0]]\n") } ;
#	$OpensslFileStore::outpath_fn = sub { System::tempfile($_[1]) } ;
#	$OpensslFileStore::inpath_fn = sub { "certificates/$_[1]" } ;
#	$OpensslFileStore::unlink_fn = sub { System::unlink($_[0]) } ;
#

use strict ;
use FileHandle ;
use File::Basename ;
use Carp ;

package OpensslFileStore ;
our $outpath_fn = sub { _joinpath( $_[0] , $_[1] ) } ;
our $inpath_fn = sub { _joinpath( $_[0] , $_[1] ) } ;
our $log_cat_fn = sub {} ;
our $unlink_fn = sub { CORE::unlink($_[0]) } ;

sub new
{
	# Constructor. The directory parameter is only used when passed to
	# the inpath and outpath functions. The filename extension parameter
	# is used for 'catfile' output filenames (eg. ".pem").
	my ( $classname , $dir , $ext ) = @_ ;
	$dir ||= "" ;
	$ext ||= "" ;
	return bless {
		m_dir => $dir ,
		m_ext => $ext ,
		m_cleanup => {} ,
	} , $classname ;
}

sub catfile
{
	# Concatenates the given files into a temporary file and the temporary file
	# is registered for cleanup with type 'cat'.
	my ( $this , @fnames ) = @_ ;
	my $dot_ext = $this->{m_ext} ;

	( my $name = join("+",@fnames) ) =~ s/\./_/g ;
	my $path_out = $this->_outfilepath( "${name}${dot_ext}" ) ;

	my @paths_in = map { $this->_infilepath($_) } @fnames ;
	_log_cat( $path_out , @paths_in ) ;
	_cat( $path_out , @paths_in ) ;

	$this->_add_cleanup( 'cat' , $path_out ) ;
	return $path_out ;
}

sub tmpfile
{
	# Returns the full path for a temporary file with the given filename key
	# and registers it for cleanup with type 'tmp'.
	my ( $this , $fname ) = @_ ;
	my $path_out = $this->_outfilepath( $fname ) ;
	$this->_add_cleanup( 'tmp' , $path_out ) ;
	return $path_out ;
}

sub outfile
{
	# Returns the full path for an output file and optionally registers it
	# for cleanup with type 'out'.
	my ( $this , $fname , $keep ) = @_ ;
	my $path_out = $this->_outfilepath( $fname ) ;
	$this->_add_cleanup( 'out' , $path_out ) unless $keep ;
	return $path_out ;
}

sub infile
{
	# Returns the full path for the given input filename. The file
	# should already exist.
	my ( $this , $fname ) = @_ ;
	my $path_in = $this->_infilepath( $fname ) ;
	die if !-f $path_in ;
	return $path_in ;
}

my $_generator = 0 ;
sub logfile
{
	# Returns the full path of a temporary log file and registers it for
	# cleanup with type 'log'. Consider customising via '$outpath_fn'
	# in order to improve uniqueness by incorporating '$$' etc.
	my ( $this ) = @_ ;
	my $fname = "$_generator.out" ; $_generator++ ;
	my $path_out = $this->_outfilepath( $fname ) ;
	$this->_add_cleanup( 'log' , $path_out ) ;
	return $path_out ;
}

sub cleanup
{
	# Deletes files having the matching cleanup type.
	my ( $this , @types ) = @_ ;
	if( !defined($types[0]) )
	{
		@types = keys %{$this->{m_cleanup}} ;
	}
	for my $type ( @types )
	{
		for my $path ( @{$this->{m_cleanup}->{$type}} )
		{
			_unlink( $path ) ;
		}
	}
}

sub _add_cleanup
{
	my ( $this , $type , $path ) = @_ ;
	$this->{m_cleanup}->{$type} ||= [] ;
	push @{$this->{m_cleanup}->{$type}} , $path ;
}

sub _joinpath
{
	return join( "/" , grep { m/./ } @_ ) ;
}

sub _cat
{
	my ( $path_out , @paths_in ) = @_ ;
	my $fh_out = new FileHandle( "$path_out" , "w" ) or die ;
	for my $path_in ( @paths_in )
	{
		my $fh_in = new FileHandle( $path_in , "r" ) or die ;
		while(<$fh_in>)
		{
			my $line = $_ ;
			print $fh_out $line or die ;
		}
		$fh_in->close() or die ;
	}
}

# --

sub _outfilepath
{
	my ( $this , $fname ) = @_ ;
	return &$outpath_fn( $this->{m_dir} , $fname ) ;
}

sub _infilepath
{
	my ( $this , $fname ) = @_ ;
	return &$inpath_fn( $this->{m_dir} , $fname ) ;
}

sub _log_cat
{
	my ( $out , @in ) = @_ ;
	&$log_cat_fn( $out , @in ) ;
}

sub _unlink
{
	my ( $path ) = @_ ;
	&$unlink_fn( $path ) ;
}

1 ;
