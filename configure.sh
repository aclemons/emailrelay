#!/bin/sh
#
# Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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
# A simple wrapper for the autoconf configure script that specifies
# more sensible directories depending on the host environment and
# simplifies cross-compilation to windows or arm.
#
# usage: configure.sh [-d] [{-o|-m|-p}] [<configure-options>]
#         -d  debug compiler flags
#         -o  openwrt sdk and uclibc (edit as required)
#         -m  mingw-w64
#         -p  rpi
#

thisdir="`cd \`dirname $0\` && pwd`"

if test ! -e "$thisdir/configure"
then
	echo error: no autoconf configure script: try running \'bootstrap\' >&2
	exit 1
fi

enable_debug=""
if test "$1" = "-d"
then
	shift
	export CFLAGS="-O0 -g"
	export CXXFLAGS="-O0 -g"
	if expr "$*" : '.*enable.debug' ; then : ; else enable_debug="--enable-debug" ; fi
:
elif expr "$*" : '.*enable.debug' >/dev/null
then
	if test "$CFLAGS$CXXFLAGS" = ""
	then
		export CFLAGS="-O0 -g"
		export CXXFLAGS="-O0 -g"
	fi
fi

if test "$1" = "-m"
then
	shift
	TARGET="i686-w64-mingw32"
	export CXX="$TARGET-g++"
	export CC="$TARGET-gcc"
	export AR="$TARGET-ar"
	export STRIP="$TARGET-strip"
	export CXXFLAGS="$CXXFLAGS -std=c++11 -pthread"
	export LDFLAGS="$LDFLAGS -pthread"
	if test -x "`which $CXX`" ; then : ; else echo "error: no mingw c++ compiler: [$CXX]\n" ; exit 1 ; fi
	$thisdir/configure $enable_debug --host $TARGET \
		--enable-windows \
		--disable-gui --without-pam --without-doxygen \
		--prefix=/usr --libexecdir=/usr/lib --sysconfdir=/etc \
		--localstatedir=/var e_initdir=/etc/init.d "$@"
:
elif test "$1" = "-p"
then
	shift
	TARGET="arm-linux-gnueabihf"
	export CXX="$TARGET-g++"
	export CC="$TARGET-gcc"
	export AR="$TARGET-ar"
	export STRIP="$TARGET-strip"
	export CXXFLAGS="$CXXFLAGS -std=c++11 -pthread"
	export LDFLAGS="$LDFLAGS -pthread"
	$thisdir/configure $enable_debug --host $TARGET \
		--disable-gui --without-pam --without-doxygen \
		--prefix=/usr --libexecdir=/usr/lib --sysconfdir=/etc \
		--localstatedir=/var e_initdir=/etc/init.d "$@"
:
elif test "$1" = "-o"
then
	shift
    TARGET="mipsel-openwrt-linux-uclibc"
    SDK_DIR="`find $HOME -maxdepth 3 -type d -iname openwrt-sdk\*uclibc\* 2>/dev/null | sort | head -1`"
    SDK_TOOLCHAIN_DIR="`find \"$SDK_DIR/staging_dir\" -type d -iname toolchain-\*uclibc\* 2>/dev/null | sort | head -1`"
    SDK_TARGET_DIR="`find \"$SDK_DIR/staging_dir\" -type d -iname target-\*uclibc\* 2>/dev/null | sort | head -1`"
    export CC="$SDK_TOOLCHAIN_DIR/bin/$TARGET-gcc"
    export CXX="$SDK_TOOLCHAIN_DIR/bin/$TARGET-c++"
    export AR="$SDK_TOOLCHAIN_DIR/bin/$TARGET-ar"
    export STRIP="$SDK_TOOLCHAIN_DIR/bin/$TARGET-strip"
    export CXXFLAGS="-fno-rtti -fno-threadsafe-statics -Os $CXXFLAGS"
    export LDFLAGS="-L$SDK_TARGET_DIR/usr/lib -luClibc++ $LDFLAGS"
    export CPPFLAGS="-I$SDK_TARGET_DIR/usr/include/uClibc++ $CPPFLAGS"
	if test -x "$CXX" ; then : ; else echo "error: no c++ compiler for target [$TARGET]: CXX=[$CXX]\n" ; exit 1 ; fi
	if test -f "$SDK_TARGET_DIR/usr/lib/libuClibc++.so" ; then : ; else echo "error: no uclibc++ library under [$SDK_TARGET_DIR]\n" ; exit 1 ; fi
    $thisdir/configure $enable_debug --host $TARGET \
		--disable-gui --without-pam --without-doxygen \
		--without-mbedtls --disable-std-thread \
		--prefix=/usr --libexecdir=/usr/lib --sysconfdir=/etc \
		--localstatedir=/var e_initdir=/etc/init.d "$@"
    echo export PATH=\"$SDK_TOOLCHAIN_DIR/bin:\$PATH\"
    echo export STAGING_DIR=\"$SDK_DIR/staging_dir\"
:
elif test "`uname`" = "NetBSD"
then
	export CPPFLAGS="$CPPFLAGS -I/usr/X11R7/include"
	export LDFLAGS="$LDFLAGS -L/usr/X11R7/lib"
	$thisdir/configure $enable_debug \
		--prefix=/usr --libexecdir=/usr/lib --sysconfdir=/etc \
		--localstatedir=/var e_bsdinitdir=/etc/rc.d "$@"
:
elif test "`uname`" = "FreeBSD"
then
	export CPPFLAGS="$CPPFLAGS -I/usr/local/include -I/usr/local/include/libav"
	export LDFLAGS="$LDFLAGS -L/usr/local/lib -L/usr/local/lib/libav"
	$thisdir/configure $enable_debug \
		--prefix=/usr/local --mandir=/usr/local/man \
		e_bsdinitdir=/usr/local/etc/rc.d "$@"
:
elif test "`uname`" = "OpenBSD"
then
	export CPPFLAGS="$CPPFLAGS -I/usr/X11R6/include"
	export LDFLAGS="$LDFLAGS -L/usr/X11R6/lib"
	$thisdir/configure $enable_debug \
		--prefix=/usr/local --mandir=/usr/local/man \
		e_bsdinitdir=/usr/local/etc/rc.d "$@"
:
elif test "`uname`" = "Darwin"
then
	export CPPFLAGS="$CPPFLAGS -I/opt/local/include -I/opt/X11/include"
	export LDFLAGS="$LDFLAGS -L/opt/local/lib -L/opt/X11/lib"
	$thisdir/configure $enable_debug \
		--prefix=/opt/local --mandir=/opt/local/man "$@"
:
elif test "`uname`" = "Linux"
then
	export CPPFLAGS
	export LDFLAGS
	$thisdir/configure $enable_debug \
		--prefix=/usr --libexecdir=/usr/lib --sysconfdir=/etc \
		--localstatedir=/var e_initdir=/etc/init.d \
		e_rundir=/run/emailrelay "$@"
:
else
	export CPPFLAGS="$CPPFLAGS -I/usr/X11R7/include -I/usr/X11R6/include -I/usr/local/include -I/opt/local/include -I/opt/X11/include"
	export LDFLAGS="$LDFLAGS -L/usr/X11R7/lib -L/usr/X11R6/lib -L/usr/local/lib -L/opt/local/lib -L/opt/X11/lib"
	$thisdir/configure $enable_debug "$@"
fi

