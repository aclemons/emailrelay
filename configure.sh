#!/bin/sh
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
# configure.sh
#
# A simple wrapper for the configure script.
#
# usage: configure.sh [-d] [-m] [<configure-options>]
#         -d  debug compiler flags (see also --enable-debug)
#         -m  mingw-w64
#

thisdir="`cd \`dirname $0\` && pwd`"

if test ! -e "$thisdir/configure"
then
	echo error: no autoconf configure script: try running \'bootstrap\' >&2
	exit 1
fi

if test "$1" = "-d"
then
	shift
	export CFLAGS="-O0 -g"
	export CXXFLAGS="-O0 -g"
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
	export CXX="i686-w64-mingw32-g++"
	export CC="i686-w64-mingw32-gcc"
	export CXXFLAGS="-std=c++11 -pthread"
	export LDFLAGS="-pthread"
	$thisdir/configure --host i686-w64-mingw32 --enable-windows --disable-gui --disable-pam \
		--prefix=/usr --libexecdir=/usr/lib --docdir=/usr/share/doc --mandir=/usr/share/man "$@"
:
elif test "`uname`" = "NetBSD"
then
	export CPPFLAGS="$CPPFLAGS -I/usr/X11R7/include"
	export LDFLAGS="$LDFLAGS -L/usr/X11R7/lib"
	$thisdir/configure --prefix=/usr --libexecdir=/usr/lib --docdir=/usr/share/doc --mandir=/usr/share/man --sysconfdir=/etc e_bsdinitdir=/etc/rc.d "$@"
:
elif test "`uname`" = "FreeBSD"
then
	export CPPFLAGS="$CPPFLAGS -I/usr/local/include -I/usr/local/include/libav"
	export LDFLAGS="$LDFLAGS -L/usr/local/lib -L/usr/local/lib/libav"
	$thisdir/configure --prefix=/usr/local --mandir=/usr/local/man e_bsdinitdir=/usr/local/etc/rc.d "$@"
:
elif test "`uname`" = "OpenBSD"
then
	export CPPFLAGS="$CPPFLAGS -I/usr/X11R6/include"
	export LDFLAGS="$LDFLAGS -L/usr/X11R6/lib"
	$thisdir/configure --prefix=/usr/local --mandir=/usr/local/man e_bsdinitdir=/usr/local/etc/rc.d "$@"
:
elif test "`uname`" = "Darwin"
then
	export CPPFLAGS="$CPPFLAGS -I/opt/local/include -I/opt/X11/include"
	export LDFLAGS="$LDFLAGS -L/opt/local/lib -L/opt/X11/lib"
	$thisdir/configure --prefix=/opt/local --mandir=/opt/local/man "$@"
:
elif test "`uname`" = "Linux"
then
	export CPPFLAGS
	export LDFLAGS
	$thisdir/configure --prefix=/usr --libexecdir=/usr/lib --sysconfdir=/etc e_initdir=/etc/init.d e_spooldir=/var/spool/emailrelay "$@"
:
else
	export CPPFLAGS="$CPPFLAGS -I/usr/X11R7/include -I/usr/X11R6/include -I/usr/local/include -I/opt/local/include -I/opt/X11/include"
	export LDFLAGS="$LDFLAGS -L/usr/X11R7/lib -L/usr/X11R6/lib -L/usr/local/lib -L/opt/local/lib -L/opt/X11/lib"
	$thisdir/configure "$@"
fi

