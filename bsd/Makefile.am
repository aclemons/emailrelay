#
## Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
## 
## This program is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
## 
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
## 
## You should have received a copy of the GNU General Public License
## along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

if GCONFIG_BSD
EXTRA_DIST = emailrelay-bsd.sh.in
e_bsdinit_SCRIPTS = emailrelay
CLEANFILES = emailrelay
else
EXTRA_DIST = emailrelay-bsd.sh.in
e_bsdinit_SCRIPTS =
CLEANFILES =
endif

do_sed = sed -e "s%__SBIN_DIR__%${sbindir}%g" -e "s%__INIT_DIR__%${e_initdir}%g"

emailrelay: emailrelay-bsd.sh.in
	$(do_sed) < $(srcdir)/emailrelay-bsd.sh.in > emailrelay
	chmod +x emailrelay

