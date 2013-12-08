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
# configure-fhs.sh
#
# A convenience script that runs the emailrelay "../configure" script with
# directory options that are more FHS-like.
#
#   $ cd src/emailrelay-1.99
#   $ mkdir build
#   $ cd build
#   $ sh ../bin/configure-fhs.sh
#   $ make 
#   $ sudo make install
#

configure="./configure"
if test ! -f "${configure}" ; then configure="../configure" ; fi
if test ! -f "${configure}" ; then configure="../../configure" ; fi
chmod +x "${configure}" 2> /dev/null

${configure} \
	--prefix=/usr \
	--exec-prefix=/usr \
	--datadir=/usr/share \
	--localstatedir=/var \
	--libexecdir=/usr/lib \
	--sysconfdir=/etc \
	e_initdir=/etc/init.d "$@"
