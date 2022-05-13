#!/bin/sh
#
# Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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
# configure.sh
#
# A wrapper for the autoconf configure script that specifies more sensible
# directories depending on the host environment and simplifies
# cross-compilation.
#
# usage: configure.sh [-g] [{-d|-s <>}] [{-o|-w|-p}] -- [<configure-options>]
#         -d   add debug compiler flags
#         -s   add sanitiser compiler flags (eg. -s address)
#         -o   cross-compile for openwrt (edit as required)
#         -w   cross-compile for windows 32-bit with mingw-w64
#         -w32 cross-compile for windows 32-bit with mingw-w64
#         -w64 cross-compile for windows 64-bit with mingw-w64
#         -p   cross-compile for rpi
#         -g   git-clone mbedtls and exit
#
# For systemd add "e_systemddir=/usr/lib/systemd/system".
#
# When cross-compiling with mbedtls the mbedtls source should be unpacked
# into this base directory (see MBEDTLS_DIR below), or use '-g' to
# git-clone it.
#

thisdir="`cd \`dirname $0\` && pwd`"

usage="[-g] [{-d|-s <>}] [{-o|-w|-p}] -- <configure-args>"
while expr "x$1" : "x-" >/dev/null
do
	valued=0
	case "`echo \"$1\" | sed 's/^--*//'`" in
		g) opt_git=1 ;;
		d) opt_debug=1 ;;
		s) opt_sanitise="$2" ; valued=1 ;;
		o) opt_openwrt=1 ;;
		w) opt_mingw=1 ; opt_win=32 ;;
		w32) opt_mingw=1 ; opt_win=32 ;;
		w64) opt_mingw=1 ; opt_win=64 ;;
		p) opt_rpi=1 ;;
		h) echo usage: `basename $0` $usage "..." ; $thisdir/configure --help=short ; exit 0 ;;
		#\?) echo usage: `basename $0` $usage >&2 ; exit 2 ;;
		*) opt_passthrough="$opt_passthrough $1" ;;
	esac
	if test $valued -eq 1 ; then shift ; fi
	shift
done
if expr 0$opt_openwrt + 0$opt_mingw + 0$opt_rpi \> 1 > /dev/null
then
	echo usage: too many target options >&2 ; exit 2
fi

if test ! -e "$thisdir/configure"
then
	echo error: no autoconf configure script: try running \'bootstrap\' >&2
	exit 1
fi

if test "0$opt_git" -eq 1
then
	git clone https://salsa.debian.org/debian/mbedtls.git
	e="$?"
	patch -d mbedtls/library -p1 < src/gssl/mbedtls-vsnprintf-fix.p1
	if test "$e" -eq 0 -a "0$opt_mingw" -eq 0
	then
		echo build with...
		echo "  make -C mbedtls/library WINDOWS=0"
	fi
	exit "$e"
fi

enable_debug=""
if test "0$opt_debug" -eq 1
then
	export CFLAGS="-O0 -g"
	export CXXFLAGS="-O0 -g"
	if expr "x$*" : '.*enable.debug' >/dev/null ; then : ; else enable_debug="--enable-debug" ; fi
:
elif expr "x$*" : '.*enable.debug' >/dev/null
then
	if test "$CFLAGS$CXXFLAGS" = ""
	then
		export CFLAGS="-O0 -g"
		export CXXFLAGS="-O0 -g"
	fi
fi

if test "$opt_sanitise" != ""
then
	export CXX="clang++"
	export CXXFLAGS="-O3 -fstrict-aliasing -Wstrict-aliasing -fsanitize=$opt_sanitise"
	export LDFLAGS="-fsanitize=$opt_sanitise"
fi

MBEDTLS_DIR="`find . -maxdepth 1 -type d -name mbedtls\* 2>/dev/null | head -1`"
make_mbedtls=0
with_mbedtls=""
if test "0$opt_mingw" -eq 1 -o "0$opt_rpi" -eq 1 -o "0$opt_openwrt" -eq 1
then
	with_mbedtls="--with-mbedtls=no"
fi
if test -d "$MBEDTLS_DIR"
then
	echo "configure.sh: mbedtls directory exists: adding --with-mbedtls and CXXFLAGS=-I$MBEDTLS_DIR/include etc"
	with_mbedtls="--with-mbedtls"
	make_mbedtls=1
	export CXXFLAGS="$CXXFLAGS -I`pwd`/$MBEDTLS_DIR/include"
	export LDFLAGS="$LDFLAGS -L`pwd`/$MBEDTLS_DIR/library"
fi

if test "0$opt_mingw" -eq 1
then
	if test "$opt_win" -eq 32
	then
		TARGET="i686-w64-mingw32" # 32-bit binaries
	else
		TARGET="x86_64-w64-mingw32" # 64-bit binaries
	fi
	export CXX="$TARGET-g++-posix"
	export CC="$TARGET-gcc-posix"
	export AR="$TARGET-ar"
	export STRIP="$TARGET-strip"
	export GCONFIG_WINDMC="$TARGET-windmc"
	export GCONFIG_WINDRES="$TARGET-windres"
	export CXXFLAGS="$CXXFLAGS -std=c++11 -pthread"
	#export CXXFLAGS="$CXXFLAGS -D_WIN32_WINNT=0x0501" eg. for Windows XP, otherwise whatever mingw defaults to
	export LDFLAGS="$LDFLAGS -pthread"
	if test -x "`which $CXX`" ; then : ; else echo "error: no mingw c++ compiler: [$CXX]\n" ; exit 1 ; fi
	( echo msbuild . ; echo qt-x86 . ; echo qt-x64 . ; echo cmake . ; echo msvc . ) > winbuild.cfg
	if test "$make_mbedtls" -eq 1
	then
		echo mbedtls $MBEDTLS_DIR >> winbuild.cfg
	else
		echo mbedtls . >> winbuild.cfg
	fi
	$thisdir/configure $enable_debug --host $TARGET \
		--enable-windows --disable-interface-names \
		$with_mbedtls \
		--disable-gui --without-pam --without-doxygen \
		--prefix=/usr --libexecdir=/usr/lib --sysconfdir=/etc \
		--localstatedir=/var $opt_passthrough e_initdir=/etc/init.d "$@"
	echo :
	echo "build with..."
	test "$make_mbedtls" -eq 1 && echo "  make -C $MBEDTLS_DIR/library WINDOWS=1 CC=$CC AR=$AR"
	echo "  make"
	echo "  make -C src/main strip"
	echo "  perl winbuild.pl mingw"
:
elif test "0$opt_rpi" -eq 1
then
	TARGET="arm-linux-gnueabihf"
	export CXX="$TARGET-g++"
	export CC="$TARGET-gcc"
	export AR="$TARGET-ar"
	export STRIP="$TARGET-strip"
	export CXXFLAGS="$CXXFLAGS -std=c++11 -pthread"
	export LDFLAGS="$LDFLAGS -pthread"
	$thisdir/configure $enable_debug --host $TARGET \
		--disable-gui $with_mbedtls --without-openssl \
		--without-pam --without-doxygen \
		--prefix=/usr --libexecdir=/usr/lib --sysconfdir=/etc \
		--localstatedir=/var $opt_passthrough e_initdir=/etc/init.d "$@"
	echo :
	echo "build with..."
	test "$make_mbedtls" -eq 1 && echo "  make -C $MBEDTLS_DIR/library CC=$CC AR=$AR"
	echo "  make"
	echo "  make -C src/main strip"
:
elif test "0$opt_openwrt" -eq 1
then
	TARGET="mips-openwrt-linux-musl"
	SDK_DIR="`find $HOME -maxdepth 3 -type d -iname openwrt-sdk\* 2>/dev/null | sort | head -1`"
	SDK_TOOLCHAIN_DIR="`find \"$SDK_DIR/staging_dir\" -type d -iname toolchain-\* 2>/dev/null | sort | head -1`"
	SDK_TARGET_DIR="`find \"$SDK_DIR/staging_dir\" -type d -iname target-\* 2>/dev/null | sort | head -1`"
	export CC="$SDK_TOOLCHAIN_DIR/bin/$TARGET-gcc"
	export CXX="$SDK_TOOLCHAIN_DIR/bin/$TARGET-c++"
	export AR="$SDK_TOOLCHAIN_DIR/bin/$TARGET-ar"
	export STRIP="$SDK_TOOLCHAIN_DIR/bin/$TARGET-strip"
	export CXXFLAGS="-fno-rtti -fno-threadsafe-statics -Os $CXXFLAGS"
	export CXXFLAGS="$CXXFLAGS -DG_SMALL"
	export LDFLAGS="$LDFLAGS -static"
	export LIBS="-lgcc_eh"
	if test -x "$CXX" ; then : ; else echo "error: no c++ compiler for target [$TARGET]: CXX=[$CXX]\n" ; exit 1 ; fi
	$thisdir/configure $enable_debug --host $TARGET \
		--disable-gui --without-pam --without-doxygen \
		$with_mbedtls --disable-std-thread \
		--prefix=/usr --libexecdir=/usr/lib --sysconfdir=/etc \
		--localstatedir=/var $opt_passthrough e_initdir=/etc/init.d "$@"
	echo :
	echo "build with..."
	#echo "  export PATH=\"$SDK_TOOLCHAIN_DIR/bin:\$PATH\""
	#echo "  export STAGING_DIR=\"$SDK_DIR/staging_dir\""
	test "$make_mbedtls" -eq 1 && echo "  make -C $MBEDTLS_DIR/library CC=$CC AR=$AR"
	echo "  make"
	echo "  make -C src/main strip"
:
elif test "`uname`" = "NetBSD"
then
	export CXXFLAGS="$CXXFLAGS -I/usr/X11R7/include"
	export LDFLAGS="$LDFLAGS -L/usr/X11R7/lib"
	$thisdir/configure $enable_debug $with_mbedtls \
		--prefix=/usr --libexecdir=/usr/lib --sysconfdir=/etc \
		--localstatedir=/var $opt_passthrough e_bsdinitdir=/etc/rc.d "$@"
:
elif test "`uname`" = "FreeBSD"
then
	export CXXFLAGS="$CXXFLAGS -I/usr/local/include -I/usr/local/include/libav"
	export LDFLAGS="$LDFLAGS -L/usr/local/lib -L/usr/local/lib/libav"
	$thisdir/configure $enable_debug $with_mbedtls \
		--prefix=/usr/local --mandir=/usr/local/man \
		$opt_passthrough e_bsdinitdir=/usr/local/etc/rc.d "$@"
:
elif test "`uname`" = "OpenBSD"
then
	export CXXFLAGS="$CXXFLAGS -I/usr/X11R6/include"
	export LDFLAGS="$LDFLAGS -L/usr/X11R6/lib"
	$thisdir/configure $enable_debug $with_mbedtls \
		--prefix=/usr/local --mandir=/usr/local/man \
		$opt_passthrough e_bsdinitdir=/usr/local/etc/rc.d "$@"
:
elif test "`uname`" = "Darwin"
then
	export CXXFLAGS="$CXXFLAGS -I/opt/local/include -I/opt/X11/include"
	export LDFLAGS="$LDFLAGS -L/opt/local/lib -L/opt/X11/lib"
	$thisdir/configure $enable_debug $with_mbedtls \
		--prefix=/opt/local --mandir=/opt/local/man $opt_passthrough "$@"
:
elif test "`uname`" = "Linux"
then
	export CXXFLAGS
	export LDFLAGS
	$thisdir/configure $enable_debug $with_mbedtls \
		--prefix=/usr --libexecdir=/usr/lib --sysconfdir=/etc \
		--localstatedir=/var e_initdir=/etc/init.d \
		$opt_passthrough e_rundir=/run/emailrelay "$@"
:
else
	export CXXFLAGS="$CXXFLAGS -I/usr/X11R7/include -I/usr/X11R6/include -I/usr/local/include -I/opt/local/include -I/opt/X11/include"
	export LDFLAGS="$LDFLAGS -L/usr/X11R7/lib -L/usr/X11R6/lib -L/usr/local/lib -L/opt/local/lib -L/opt/X11/lib"
	$thisdir/configure $enable_debug $with_mbedtls $opt_passthrough "$@"
fi

