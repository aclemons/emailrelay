#!/bin/sh
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
#         -m   git fetch mbedtls v2
#         -m3  git fetch mbedtls v3
#         -q6  build gui using ~/Qt/6.x.x
#         -S   force e_systemddir
#         -X   suppress e_systemddir
#
# When cross-compiling with mbedtls the mbedtls source should be unpacked
# into this base directory (see MBEDTLS_DIR below), or use '-g' to
# git-clone it.
#

thisdir="`cd \`dirname $0\` && pwd`"

usage="[-g] [{-d|-s <>}] [{-o|-w|-p}] -- <configure-args>"
opt_systemd=0 ; if test "`systemctl is-system-running 2>/dev/null | sed 's/offline//'`" != "" ; then opt_systemd=1 ; fi
while expr "x$1" : "x-" >/dev/null
do
	valued=0
	case "`echo \"$1\" | sed 's/^--*//'`" in
		m|m2|g) opt_get_mbedtls=2 ;;
		m3) opt_get_mbedtls=3 ;;
		d) opt_debug=1 ;;
		s) opt_sanitise="$2" ; valued=1 ;;
		o) opt_openwrt=1 ;;
		w) opt_mingw=1 ; opt_win=32 ;;
		w32) opt_mingw=1 ; opt_win=32 ;;
		w64) opt_mingw=1 ; opt_win=64 ;;
		p) opt_rpi=1 ;;
		q6|qt6) opt_qt6_home=1 ;;
		S) opt_systemd=1 ;;
		X) opt_systemd=0 ;;
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

enable_debug=""
if test "0$opt_debug" -ne 0
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

if test "0$opt_get_mbedtls" -ne 0
then
	if test -d mbedtls
	then
		git -C mbedtls fetch
	else
    	git clone https://github.com/Mbed-TLS/mbedtls.git mbedtls
	fi
	if test "$opt_get_mbedtls" -eq 3
	then
    	git -C mbedtls checkout -q "mbedtls-3.5.0"
	else
		git -C mbedtls checkout -q "mbedtls-2.28"
	fi
	if test "0$opt_mingw" -ne 0
	then
		if test "$opt_win" -eq 32
		then
			patch -d mbedtls/library -l -N -r - -p1 < src/gssl/mbedtls-vsnprintf-fix.p1
			patch -d mbedtls/library -l -N -r - -p1 < src/gssl/mbedtls-vsnprintf-fix-new.p1
			rm mbedtls/library/platform.c.orig 2>/dev/null
		fi
	fi
fi

MBEDTLS_DIR="`find . -maxdepth 1 -type d -name mbedtls\* 2>/dev/null | head -1`"
make_mbedtls=0
configure_mbedtls=""
if test "0$opt_mingw" -ne 0 -o "0$opt_rpi" -ne 0 -o "0$opt_openwrt" -ne 0
then
	configure_mbedtls="--with-mbedtls=no"
fi
if test -d "$MBEDTLS_DIR"
then
	echo "configure.sh: mbedtls exists: adding --with-mbedtls and CXXFLAGS=-I$MBEDTLS_DIR/include etc"
	configure_mbedtls="--with-mbedtls"
	make_mbedtls=1
	export CXXFLAGS="$CXXFLAGS -I`pwd`/$MBEDTLS_DIR/include"
	export LDFLAGS="$LDFLAGS -L`pwd`/$MBEDTLS_DIR/library"
fi

MakeTlsCommand()
{
	if test "0$make_mbedtls" -ne 0
	then
		if test "0$opt_mingw" -ne 0
		then
			echo "make -C $MBEDTLS_DIR/library WINDOWS=1" "$@" PYTHON=python3
		else
			echo "make -C $MBEDTLS_DIR/library" "$@"
		fi
	fi
}

Echo()
{
	if test "`echo x $*`" != "x"
	then
		echo "$@"
	fi
}

if test "0$opt_qt6_home" -ne 0
then
	qt_dir="`ls -1td \"$HOME\"/Qt/6*/gcc_64 2>/dev/null | head -1`"
	if test ! -d "$qt_dir" ; then echo configure.sh: no qt6 directory $qt_dir >&2 ; exit 1 ; fi
	export LDFLAGS="$LDFLAGS -L$qt_dir/lib"
	export QT_LIBS="-lQt6Gui -lQt6Widgets -lQt6Core"
	export QT_CFLAGS="-std=c++17 -I$qt_dir/include"
	export QT_MOC="$qt_dir/libexec/moc"
	configure_qt="--enable-gui"
	echo "configure.sh: qt: adding --enable-gui and QT_MOC=$qt_dir/libexec/moc etc"
fi

if test "0$opt_mingw" -ne 0
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
	if test "$make_mbedtls" -ne 0
	then
		echo mbedtls $MBEDTLS_DIR >> winbuild.cfg
	else
		echo mbedtls . >> winbuild.cfg
	fi
	$thisdir/configure $enable_debug --host $TARGET \
		--enable-windows --disable-interface-names \
		$configure_mbedtls \
		--disable-gui --without-pam --without-doxygen \
		--prefix=/usr --libexecdir=/usr/lib --sysconfdir=/etc \
		--localstatedir=/var $opt_passthrough e_initdir=/etc/init.d "$@"
	echo :
	echo "build with..."
	Echo "  `MakeTlsCommand CC=$CC AR=$AR`"
	echo "  make"
	echo "  make -C src/main strip"
	echo "  perl winbuild.pl install_winxp"
:
elif test "0$opt_rpi" -ne 0
then
	TARGET="arm-linux-gnueabihf"
	export CXX="$TARGET-g++"
	export CC="$TARGET-gcc"
	export AR="$TARGET-ar"
	export STRIP="$TARGET-strip"
	export CXXFLAGS="$CXXFLAGS -std=c++11 -pthread -Wno-psabi"
	export LDFLAGS="$LDFLAGS -pthread"
	$thisdir/configure $enable_debug --host $TARGET \
		--disable-gui $configure_mbedtls --without-openssl \
		--without-pam --without-doxygen \
		--prefix=/usr --libexecdir=/usr/lib --sysconfdir=/etc \
		--localstatedir=/var $opt_passthrough e_initdir=/etc/init.d "$@"
	echo :
	echo "build with..."
	Echo "  `MakeTlsCommand CC=$CC AR=$AR`"
	echo "  make"
	echo "  make -C src/main strip"
:
elif test "0$opt_openwrt" -ne 0
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
	export CXXFLAGS="$CXXFLAGS -DG_LIB_SMALL -DG_SMALL"
	export LDFLAGS="$LDFLAGS -static"
	export LIBS="-lgcc_eh"
	if test -x "$CXX" ; then : ; else echo "error: no c++ compiler for target [$TARGET]: CXX=[$CXX]\n" ; exit 1 ; fi
	$thisdir/configure $enable_debug --host $TARGET \
		--disable-gui --without-pam --without-doxygen \
		$configure_mbedtls --disable-std-thread \
		--prefix=/usr --libexecdir=/usr/lib --sysconfdir=/etc \
		--localstatedir=/var $opt_passthrough e_initdir=/etc/init.d "$@"
	echo :
	echo "build with..."
#	echo "  export PATH=\"$SDK_TOOLCHAIN_DIR/bin:\$PATH\""
#	echo "  export STAGING_DIR=\"$SDK_DIR/staging_dir\""
	Echo "  `MakeTlsCommand CC=$CC AR=$AR`"
	echo "  make"
	echo "  make -C src/main strip"
:
elif test "`uname`" = "NetBSD"
then
	export CXXFLAGS="$CXXFLAGS -I/usr/X11R7/include"
	export LDFLAGS="$LDFLAGS -L/usr/X11R7/lib"
	$thisdir/configure $enable_debug $configure_mbedtls \
		--prefix=/usr --libexecdir=/usr/lib --sysconfdir=/etc \
		--localstatedir=/var $opt_passthrough e_bsdinitdir=/etc/rc.d "$@"
:
elif test "`uname`" = "FreeBSD"
then
	export CXXFLAGS="$CXXFLAGS -I/usr/local/include -I/usr/local/include/libav"
	export LDFLAGS="$LDFLAGS -L/usr/local/lib -L/usr/local/lib/libav"
	$thisdir/configure $enable_debug $configure_mbedtls \
		--prefix=/usr/local --mandir=/usr/local/man \
		$opt_passthrough e_bsdinitdir=/usr/local/etc/rc.d "$@"
:
elif test "`uname`" = "OpenBSD"
then
	export CXXFLAGS="$CXXFLAGS -I/usr/X11R6/include"
	export LDFLAGS="$LDFLAGS -L/usr/X11R6/lib"
	$thisdir/configure $enable_debug $configure_mbedtls \
		--prefix=/usr/local --mandir=/usr/local/man \
		$opt_passthrough e_bsdinitdir=/usr/local/etc/rc.d "$@"
:
elif test "`uname`" = "Darwin"
then
	export CXXFLAGS="$CXXFLAGS -I/opt/local/include -I/opt/X11/include"
	export LDFLAGS="$LDFLAGS -L/opt/local/lib -L/opt/X11/lib"
	$thisdir/configure $enable_debug $configure_mbedtls \
		--prefix=/opt/local --mandir=/opt/local/man $opt_passthrough "$@"
:
elif test "`uname`" = "Linux" -a "$opt_systemd" -ne 0
then
	export CXXFLAGS
	export LDFLAGS
	$thisdir/configure $enable_debug $configure_mbedtls $configure_qt \
		--prefix=/usr --libexecdir=/usr/lib --sysconfdir=/etc \
		--localstatedir=/var e_systemddir=/usr/lib/systemd/system \
		$opt_passthrough e_rundir=/run/emailrelay "$@"
:
elif test "`uname`" = "Linux"
then
	export CXXFLAGS
	export LDFLAGS
	$thisdir/configure $enable_debug $configure_mbedtls $configure_qt \
		--prefix=/usr --libexecdir=/usr/lib --sysconfdir=/etc \
		--localstatedir=/var e_initdir=/etc/init.d \
		$opt_passthrough e_rundir=/run/emailrelay "$@"
	if test "0$opt_get_mbedtls" -ne 0
	then
		echo :
		echo "build with..."
		Echo "  `MakeTlsCommand`"
		echo "  make"
	fi
:
else
	export CXXFLAGS="$CXXFLAGS -I/usr/X11R7/include -I/usr/X11R6/include -I/usr/local/include -I/opt/local/include -I/opt/X11/include"
	export LDFLAGS="$LDFLAGS -L/usr/X11R7/lib -L/usr/X11R6/lib -L/usr/local/lib -L/opt/local/lib -L/opt/X11/lib"
	$thisdir/configure $enable_debug $configure_mbedtls $configure_qt $opt_passthrough "$@"
fi

