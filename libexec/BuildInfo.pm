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
# BuildInfo.pm
#
# Extracts and enhances information read from the emailrelay
# automake makefiles.
#
# Synopis:
#
#  use BuildInfo ;
#  my @m = BuildInfo::read_makefiles( "./emailrelay" , "reading: " ,
#      {windows=>1,windows_gui=>1,windows_mbedtls=>1,windows_openssl=>0,qt_version=>5} ) ;
#  for my $m ( @m )
#  {
#    print $m->{e_whatever} , "\n" ;
#  }
#  Build::dump( @m ) ;
#

use strict ;
use Cwd ;
use File::Basename ;
use File::Glob ;
use FileHandle ;
use Data::Dumper ;
use lib( File::Basename::dirname($0) ) ;
use AutoMakeParser ;
use ConfigStatus ;
$AutoMakeParser::debug = 0 ;

package BuildInfo ;

sub read_makefiles
{
	my ( $base , $prefix , $opt_in ) = @_ ;

	$base ||= "." ;
	$prefix ||= "reading: " ;
	$opt_in ||= {} ;

	my $opt_for_windows = exists($opt_in->{windows}) ? $opt_in->{windows} : ( $^O ne "linux" ) ;
	my $opt_windows_mbedtls = $opt_in->{windows_mbedtls} ;
	my $opt_windows_openssl = $opt_in->{windows_openssl} ;
	my $opt_windows_gui = $opt_in->{windows_gui} ;
	my $opt_qt_version = $opt_in->{qt_version} || "5" ;
	my $opt_verbose_parser = $opt_in->{verbose} ;

	if( ! -e "$base/src/glib/gdef.h" )
	{
		die "${prefix}error: no source at [$base] ($base/src/glib/gdef.h)\n" ;
	}

	my %switches = () ;
	my %vars = () ;
	if( $opt_for_windows )
	{
		%switches = windows_switches( $opt_windows_mbedtls , $opt_windows_openssl , $opt_windows_gui ) ;
		%vars = windows_vars() ;
	}
	else
	{
		my $cs = new ConfigStatus({dir=>$base}) ;
		%switches = $cs->switches() ;
		%vars = $cs->vars() ;
		$vars{top_srcdir} = "." ;
		$vars{top_builddir} = "." ;
	}
	my $cfg_with_mbedtls = $switches{GCONFIG_TLS_USE_MBEDTLS} || $switches{GCONFIG_TLS_USE_BOTH} ;
	my $cfg_with_openssl = $switches{GCONFIG_TLS_USE_OPENSSL} || $switches{GCONFIG_TLS_USE_BOTH} ;
	my $cfg_with_qt = $switches{GCONFIG_GUI} ;

	my @m = AutoMakeParser::readall( $base , \%switches , \%vars , $opt_verbose_parser , $prefix ) ;
	my $first = 1 ;
	for my $m ( @m )
	{
		$m->{e_project} = "emailrelay" ;
		$m->{e_is_windows} = $opt_for_windows ;
		$m->{e_with_mbedtls} = $cfg_with_mbedtls ; # 'with' => project-wide
		$m->{e_with_openssl} = $cfg_with_openssl ;
		$m->{e_with_qt} = $cfg_with_qt ;
		$m->{e_dir} = File::Basename::dirname($m->path()) ;
		$m->{e_dirname} = File::Basename::basename(File::Basename::dirname($m->path())) ;
		$m->{e_is_top_dir} = $first ; $first = 0 ;
		$m->{e_is_gui_dir} = $m->{e_dirname} eq "gui" ;
		$m->{e_is_src_dir} = $m->{e_dirname} eq "src" ;
		$m->{e_cmake_out} = join( "/" , $m->{e_dir} , "CMakeLists.txt" ) ;
		$m->{e_qmake_out} = $m->{e_is_top_dir} ? "$$m{e_project}.pro" : "$$m{e_dir}/$$m{e_dirname}.pro" ;
		if( $opt_for_windows )
		{
			$m->{e_compile_options} = "" ;
		}
		else
		{
			$m->{e_compile_options} = $m->compile_options() ;
			$m->{e_compile_options} .= " -std=c++11" if ( $m->{e_compile_options} !~ m/std=c++/ ) ;
			$m->{e_compile_options} .= " -fPIC" if $m->{e_is_gui_dir} ;
		}
		$m->{e_copy} = $opt_for_windows ? "copy /y" : "cp" ;
		$m->{e_need_mbedtls_inc} = $cfg_with_mbedtls && $m->{e_dirname} eq "gssl" ;
		$m->{e_need_mbedtls_libs} = undef ; # see below
		$m->{e_need_openssl_inc} = $cfg_with_openssl && $m->{e_dirname} eq "gssl" ;
		$m->{e_need_openssl_libs} = undef ; # see below
		$m->{e_need_qt_inc} = $cfg_with_qt && $m->{e_is_gui_dir} ;
		$m->{e_need_qt_libs} = undef ; # see below
		$m->{e_qt_version} = $opt_qt_version ;

		my @mbedtls_libnames = qw( mbedtls mbedx509 mbedcrypto ) ;
		my @mbedtls_sys_libnames = () ;
		push @mbedtls_sys_libnames , "bcrypt" if( $opt_for_windows ) ; # mbedtls v3.x

		my @openssl_libnames = qw( ssl crypto ) ;

		my @qt_libnames_release = ( "Qt${opt_qt_version}Widgets" , "Qt${opt_qt_version}Gui" , "Qt${opt_qt_version}Core" ) ;
		push @qt_libnames_release , "qtmain" if $opt_for_windows ;
		my @qt_libnames_debug = map { $_."d" } @qt_libnames_release ;
		push @qt_libnames_debug , "qtmaind" if $opt_for_windows ;
		my @qt_static_libnames_release = (
			'../plugins/styles/qwindowsvistastyle' ,
			'../plugins/platforms/qwindows' ,
			#'../plugins/imageformats/qgif' ,
			'../plugins/imageformats/qico' ,
			#'../plugins/imageformats/qjpeg' ,
			"Qt${opt_qt_version}EventDispatcherSupport" ,
			"Qt${opt_qt_version}FontDatabaseSupport" ,
			"Qt${opt_qt_version}ThemeSupport" ,
			"Qt${opt_qt_version}AccessibilitySupport" ,
			"Qt${opt_qt_version}WindowsUIAutomationSupport" ,
			'qtfreetype' ,
			'qtpcre2' ,
			#'qtlibpng' ,
			'qtharfbuzz' ,
		) ;
		my @qt_static_libnames_debug = map {"${_}d"} @qt_static_libnames_release ;
		my @qt_static_sys_libnames = () ;
		if( $opt_for_windows )
		{
			push @qt_static_sys_libnames , qw(
				dwmapi dwrite dxgi dxguid
				d2d1 d3d11 imm32 netapi32 ole32
				oleaut32 shlwapi userenv uxtheme version
				winmm winspool wtsapi32 gdi32
			) ;
		}

		$m->{e_mbedtls_libnames} = \@mbedtls_libnames ;
		$m->{e_openssl_libnames} = \@openssl_libnames ;
		$m->{e_qt_libnames_debug} = \@qt_libnames_debug ;
		$m->{e_qt_libnames_release} = \@qt_libnames_release ;
		$m->{e_qt_static_libnames_debug} = \@qt_static_libnames_debug ;
		$m->{e_qt_static_libnames_release} = \@qt_static_libnames_release ;
		$m->{e_qt_static_sys_libnames} = \@qt_static_sys_libnames ; # additional to sys_libnames

		my @subdirs = $m->subdirs() ;
		$m->{e_subdirs} = \@subdirs ;

		my @definitions = $m->definitions() ;
		push @definitions , qw(QT_WIDGETS_LIB QT_GUI_LIB QT_CORE_LIB) if $m->{e_need_qt_inc} ;
		if( $opt_for_windows )
		{
			@definitions = grep{!m/G_LIB_SMALL/} @definitions ; # TODO also no lib-small for gui
			push @definitions , "G_WINDOWS=1" ;
			push @definitions , "GCONFIG_NO_GCONFIG_DEFS=1" ;
		}
		$m->{e_definitions} = \@definitions ;

		my @includes = ( "." , ".." , $m->includes($m->base()) ) ;
		$m->{e_includes} = \@includes ;

		if( !$cfg_with_qt )
		{
			# dont build anything in src/gui
			if( $m->{e_is_src_dir} )
			{
				@subdirs = grep {$_ ne "gui"} @subdirs ;
			}
			elsif( $m->{e_is_gui_dir} )
			{
				$m->{e_libraries} = [] ;
				$m->{e_programs} = [] ;
				next ; # no libraries, no programs
			}
		}

		my @libraries = map {my $x=$_; $x =~ s/^lib//r =~ s/\.a$//r } $m->libraries() ;
		$m->{e_libraries} = \@libraries ;
		for my $library ( @libraries )
		{
			my $dotobj = $opt_for_windows ? ".obj" : ".o" ;
			my @sources = $m->sources( "lib$library.a" ) ; # windows sic
			@sources = () if( $library =~ m/extra$/ ) ;
			my @objects = map {my $x=$_;$x=~ s/\.cp*$/$dotobj/ ;$x} @sources ;

			$m->{e_library} ||= {} ;
			$m->{e_library}->{$library} ||= {} ;
			$m->{e_library}->{$library}->{libfile} = $opt_for_windows ? "$library.lib" : "lib$library.a" ;
			$m->{e_library}->{$library}->{libname} = $library ;
			$m->{e_library}->{$library}->{sources} = \@sources ;
			$m->{e_library}->{$library}->{objects} = \@objects ;
			$m->{e_library}->{$library}->{compile_options} = $m->{e_compile_options} ;
			$m->{e_library}->{$library}->{includes} = \@includes ;
			$m->{e_library}->{$library}->{definitions} = \@definitions ;
			$m->{e_library}->{$library}->{need_mbedtls_inc} = $m->{e_need_mbedtls_inc} ;
			$m->{e_library}->{$library}->{need_openssl_inc} = $m->{e_need_openssl_inc} && ( $library ne "gsslkeygen" ) ;
			$m->{e_library}->{$library}->{need_qt_inc} = $m->{e_need_qt_inc} ;
		}

		my @programs = $m->programs() ;
		$m->{e_programs} = \@programs ;
		for my $program ( @programs )
		{
			my $mkey = $program ; # fwiw
			my $dotobj = $opt_for_windows ? ".obj" : ".o" ;
			my $dotexe = $opt_for_windows ? ".exe" : "" ;
			my @sources = grep {$_ ne "$mkey.rc"} $m->sources( $mkey ) ;
			my @objects = map {my $x=$_;$x=~ s/\.cp*$/$dotobj/ ;$x} @sources ;
			my @our_libnames = $m->our_libnames( $mkey ) ; # "bar"
			my @our_libdirs = $m->our_libdirs( $mkey , $m->base() , $m->{e_dir} ) ; # "build-base/dir/foo"
			my @sys_libnames = # comctl32, pam etc
				grep {!(m/^mbed/)}
				grep {!(m/^ssl|^crypto/)}
				grep {!(m/^Qt/i)}
				$m->sys_libs( $mkey ) ;
			unshift @sys_libnames , @mbedtls_sys_libnames if $cfg_with_mbedtls ;

			my $need_mbedtls_libs = $cfg_with_mbedtls && scalar( grep{$_ eq "gssl" || $_ eq "gsslkeygen"} @our_libnames ) ;
			$m->{e_need_mbedtls_libs} ||= $need_mbedtls_libs ;
			my $need_openssl_libs = $cfg_with_openssl && scalar( grep{$_ eq "gssl"} @our_libnames ) ;
			$m->{e_need_openssl_libs} ||= $need_openssl_libs ;
			my $need_qt_libs = $cfg_with_qt && $m->{e_is_gui_dir} && ( $program ne "emailrelay-keygen" ) ;
			$m->{e_need_qt_libs} ||= $need_qt_libs ;

			my $manifest = "$program.exe.manifest" if( grep {m/${program}\.exe\.manifest/} ($m->value("EXTRA_DIST"),$m->sources($mkey)) ) ;
			my $rcfile = "${mkey}.rc" if( grep {$_ eq "$mkey.rc"} $m->value("EXTRA_DIST") ) ;
			my $messages_mc = "messages.mc" if ( $mkey eq "emailrelay" ) ;
			my $binfile = $messages_mc ? "MSG00001.bin" : undef ;
			my @moc_out = $m->value( "MOC_OUTPUT" ) ;
			my @moc_in = map {my $x=$_;$x=$x =~ s/\.cpp$/.h/r =~ s/^moc_//r ;$x} @moc_out ;
			my @link_options = $opt_for_windows ? () : $m->link_options() ;
			my $uac = undef ;
			$uac = "asInvoker" if( $opt_for_windows && ( $program eq "emailrelay" ) ) ;
			$uac = "requireAdministrator" if( $opt_for_windows && ( $program eq "emailrelay-service" ) ) ;
			$uac = "highestAvailable" if( $opt_for_windows && ( $program eq "emailrelay-gui" ) ) ;
			my $uac_option = ( $uac eq "no" ? "/MANIFESTUAC:NO" : "\"/MANIFESTUAC:level='$uac'\"" ) if $uac ;
			my $commoncontrols_option = '"' .
				"/MANIFESTDEPENDENCY:type='win32' name='Microsoft.Windows.Common-Controls' " .
				"version='6.0.0.0' publicKeyToken='6595b64144ccf1df' language='*' processorArchitecture='*'" . '"' ;
			my $uses_commoncontrols = $opt_for_windows && ( $program eq "emailrelay" || $program eq "emailrelay-gui" ) ;

			my @our_libpairs = () ;
			for my $i ( 0 .. scalar(@our_libnames)-1 )
			{
				push @our_libpairs , [ $our_libdirs[$i] , $our_libnames[$i] ] ;
			}

			$m->{e_program} ||= {} ;
			$m->{e_program}->{$program} ||= {} ;
			$m->{e_program}->{$program}->{progfile} = "${program}${dotexe}" ;
			$m->{e_program}->{$program}->{progname} = $program ;
			$m->{e_program}->{$program}->{sources} = \@sources ;
			$m->{e_program}->{$program}->{objects} = \@objects ;
			$m->{e_program}->{$program}->{our_libpairs} = \@our_libpairs ;
			$m->{e_program}->{$program}->{our_libnames} = \@our_libnames ;
			$m->{e_program}->{$program}->{our_libdirs} = \@our_libdirs ;
			$m->{e_program}->{$program}->{need_mbedtls_libs} = $need_mbedtls_libs ; # add mbedtls_libnames
			$m->{e_program}->{$program}->{need_openssl_libs} = $need_openssl_libs ; # add openssl_libnames
			$m->{e_program}->{$program}->{need_qt_libs} = $need_qt_libs ; # add $qt_libnames_debug/release
			$m->{e_program}->{$program}->{sys_libnames} = \@sys_libnames ;
			$m->{e_program}->{$program}->{mbedtls_libnames} = $m->{e_mbedtls_libnames} ;
			$m->{e_program}->{$program}->{openssl_libnames} = $m->{e_openssl_libnames} ;
			$m->{e_program}->{$program}->{qt_libnames_debug} = $need_qt_libs ? $m->{e_qt_libnames_debug} : [] ;
			$m->{e_program}->{$program}->{qt_libnames_release} = $need_qt_libs ? $m->{e_qt_libnames_release} : [] ;
			$m->{e_program}->{$program}->{qt_static_libnames_debug} = $need_qt_libs ? $m->{e_qt_static_libnames_debug} : [] ;
			$m->{e_program}->{$program}->{qt_static_libnames_release} = $need_qt_libs ? $m->{e_qt_static_libnames_release} : [] ;
			$m->{e_program}->{$program}->{qt_static_sys_libnames} = $need_qt_libs ? $m->{e_qt_static_sys_libnames} : [] ;
			$m->{e_program}->{$program}->{subsystem} = ($need_qt_libs || $program eq "emailrelay") ? "windows" : "console" ;
			$m->{e_program}->{$program}->{messages_mc} = $opt_for_windows ? $messages_mc : "" ;
			$m->{e_program}->{$program}->{rcfile} = $opt_for_windows ? $rcfile : "" ;
			$m->{e_program}->{$program}->{binfile} = $opt_for_windows ? $binfile : "" ;
			$m->{e_program}->{$program}->{moc_in} = \@moc_in ;
			$m->{e_program}->{$program}->{moc_out} = \@moc_out ;
			$m->{e_program}->{$program}->{link_options} = \@link_options ;
			$m->{e_program}->{$program}->{compile_options} = $m->{e_compile_options} ;
			$m->{e_program}->{$program}->{includes} = $m->{e_includes} ;
			$m->{e_program}->{$program}->{definitions} = $m->{e_definitions} ;
			$m->{e_program}->{$program}->{manifest} = $manifest if $opt_for_windows ;
			$m->{e_program}->{$program}->{uac} = $uac if $opt_for_windows ;
			$m->{e_program}->{$program}->{uac_option} = $uac_option if $opt_for_windows ;
			$m->{e_program}->{$program}->{uses_commoncontrols} = $uses_commoncontrols if $opt_for_windows ;
			$m->{e_program}->{$program}->{commoncontrols_option} = $commoncontrols_option if $opt_for_windows ;
		}
	}
	return @m ;
}

sub qt_version
{
	my ( $qt_root ) = @_ ;
	return undef if !$qt_root ;
	my $fh = new FileHandle( "$qt_root/include/QtCore/qtcoreversion.h" ) ;
	my $s = eval { local $/ ; <$fh> } ;
	return 5 if ( $s =~ m/#define +QTCORE_VERSION_STR +.5/m ) ;
	return 6 if ( $s =~ m/#define +QTCORE_VERSION_STR +.6/m ) ;
	return undef ;
}

sub qt_is_static
{
	my ( $qt_root ) = @_ ;
	return undef if !$qt_root ;
	my @lib_glob = File::Glob::bsd_glob( "$qt_root/lib/Qt*Core.dll" ) ;
	my @bin_glob = File::Glob::bsd_glob( "$qt_root/bin/Qt*Core.dll" ) ;
	return (scalar(@lib_glob)+scalar(@bin_glob)) == 0 ;
}

sub dump
{
	my ( @m ) = @_ ;
	# prune leaving only "e_..." members
	for my $m ( @m )
	{
		for my $k ( keys %$m )
		{
			$m->{$k} = undef if( $k !~ m/e_/ ) ;
		}
	}
	dumpall( @m ) ;
}

sub dumpall
{
	my ( @m ) = @_ ;
	my $d = new Data::Dumper( \@m ) ;
	$d->Sortkeys( 1 ) ;
	$d->Deepcopy( 1 ) ;
	print $d->Dump() , "\n" ;
}

sub windows_switches
{
	my ( $mbedtls , $openssl , $gui ) = @_ ;
	return (
		GCONFIG_BSD => 0 ,
		GCONFIG_DNSBL => 1 ,
		GCONFIG_EPOLL => 0 ,
		GCONFIG_GETTEXT => 0 ,
		GCONFIG_GUI => ($gui?1:0) ,
		GCONFIG_ICONV => 0 ,
		GCONFIG_INSTALL_HOOK => 0 ,
		GCONFIG_INTERFACE_NAMES => 1 ,
		GCONFIG_MAC => 0 ,
		GCONFIG_PAM => 0 ,
		GCONFIG_POP => 1 ,
		GCONFIG_ADMIN => 1 ,
		GCONFIG_TESTING => 1 ,
		GCONFIG_TLS_USE_MBEDTLS => (($mbedtls&&!$openssl)?1:0) ,
		GCONFIG_TLS_USE_OPENSSL => ((!$mbedtls&&$openssl)?1:0) ,
		GCONFIG_TLS_USE_BOTH => (($mbedtls&&$openssl)?1:0) ,
		GCONFIG_TLS_USE_NONE => ((!$mbedtls&&!$openssl)?0:1) ,
		GCONFIG_UDS => 0 ,
		GCONFIG_WINDOWS => 1 ,
	) ;
}

sub windows_vars
{
	return (
		top_srcdir => "." ,
		top_builddir => "." ,
		sbindir => "." ,
		mandir => "." ,
		localedir => "." ,
		e_spooldir => "c:/emailrelay" , # passed as -D but not used -- see src/gstore/gfilestore_win32.cpp
		e_docdir => "c:/emailrelay" ,
		e_initdir => "c:/emailrelay" ,
		e_bsdinitdir => "c:/emailrelay" ,
		e_rundir => "c:/emailrelay" ,
		e_icondir => "c:/emailrelay" ,
		e_trdir => "c:/emailrelay" ,
		e_examplesdir => "c:/emailrelay" ,
		e_libdir => "c:/emailrelay" ,
		e_pamdir => "c:/emailrelay" ,
		e_sysconfdir => "c:/emailrelay" ,
		GCONFIG_WINDRES => "windres" ,
		GCONFIG_WINDMC => "mc" ,
		GCONFIG_QT_LIBS => "" ,
		GCONFIG_QT_CFLAGS => "-DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB" ,
		GCONFIG_QT_MOC => "" ,
		GCONFIG_TLS_LIBS => "-lmbedtls -lmbedx509 -lmbedcrypto" ,
		GCONFIG_STATIC_START => "" ,
		GCONFIG_STATIC_END => "" ,
		VERSION => "1.0" ,
		RPM_ARCH => "x86" ,
		RPM_ROOT => "rpm" ,
	) ;
}

1 ;
