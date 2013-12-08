#!/bin/sh
#
# Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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
# configure-dev.sh
#
# A convenience script that runs the emailrelay "../configure" script 
# for developers.
#

configure="./configure"
if test ! -f "${configure}" ; then configure="../configure" ; fi
if test ! -f "${configure}" ; then configure="../../configure" ; fi
chmod +x "${configure}" 2> /dev/null

gnu()
{
	env \
		CPPFLAGS="-D_FORTIFY_SOURCE=2" \
		CXXFLAGS="-Wall -Wextra -pedantic -g -O0 -fno-omit-frame-pointer -fstack-protector" \
		CFLAGS="-Wall -Wextra -pedantic -g -O0 -fstack-protector" \
		LDFLAGS="-g" \
		"$@"
}

llvm()
{
	w_on="-Wall -Wextra -Weverything -pedantic"
	w_off="-Wno-weak-vtables -Wno-padded -Wno-exit-time-destructors -Wno-missing-noreturn -Wno-global-constructors"
	#sanity="-fsanitize=address -fsanitize=init-order -fsanitize=integer -fsanitize=undefined -fsanitize=dataflow"
	env \
		CC=clang \
		CXX=clang++ \
		CPP="clang -E" \
		LD=clang++ \
		CPPFLAGS="-D_FORTIFY_SOURCE=2" \
		CXXFLAGS="$w_on $w_off -g -O0 -fno-omit-frame-pointer -fstack-protector $sanity" \
		CFLAGS="$w_on -g -O0 -fstack-protector" \
		LDFLAGS="-g" \
		"$@"
}

args()
{
	echo \
	--enable-debug \
	--prefix=$HOME/tmp/usr \
	--exec-prefix=$HOME/tmp/usr \
	--datadir=$HOME/tmp/usr/share \
	--localstatedir=$HOME/tmp/var \
	--libexecdir=$HOME/tmp/usr/lib \
	--sysconfdir=$HOME/etc \
	e_initdir=$HOME/etc/init.d
}

#gnu ${configure} `args` "$@"
llvm ${configure} `args` "$@"

