#
## Copyright (C) 2001-2008 Graeme Walker <graeme_walker@users.sourceforge.net>
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

EXTRA_DIST = emailrelay.conf.template

e_sysconf_DATA = emailrelay.conf.template

install-data-hook:
	if test -f "$(DESTDIR)$(e_sysconfdir)/emailrelay.conf" ; then : ; else cat "$(DESTDIR)$(e_sysconfdir)/emailrelay.conf.template" | sed "s:^#spool-dir /var/spool/emailrelay:spool-dir $(DESTDIR)$(e_spooldir):g" | sed 's:"/var/spool/emailrelay":"'"$(e_spooldir)"'":g' | sed "s:/etc:$(e_sysconfdir):g" | sed "s:/usr/local/bin:$(e_libexecdir):g" > "$(DESTDIR)$(e_sysconfdir)/emailrelay.conf" ; fi

