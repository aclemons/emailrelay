#
## Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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

if GCONFIG_PAM
 e_pam_DATA = emailrelay
else
 e_pam_DATA =
endif

EXTRA_DIST = \
	emailrelay.conf.template \
	emailrelay.auth.template \
	emailrelay.pam

e_sysconf_DATA = \
	emailrelay.conf.template \
	emailrelay.auth.template

emailrelay: emailrelay.pam
	cp $(top_srcdir)/etc/emailrelay.pam $@

clean-local:
	-rm -f emailrelay 2>/dev/null

if GCONFIG_INSTALL_HOOK

# create the .conf file from .conf.template with directory edits - disable
# this when running configure for a .deb or .rpm package by using
# 'configure --disable-install-hook'
install-data-hook:
	if test -f "$(DESTDIR)$(e_sysconfdir)/emailrelay.conf" ; then : ; else cat "$(DESTDIR)$(e_sysconfdir)/emailrelay.conf.template" | sed "s:^#spool-dir .*:spool-dir $(e_spooldir):g" | sed 's:"/var/spool/emailrelay":"'"$(e_spooldir)"'":g' | sed "s:/etc:$(e_sysconfdir):g" | sed "s:/usr/local/bin:$(e_libexecdir):g" > "$(DESTDIR)$(e_sysconfdir)/emailrelay.conf" ; fi
	cp "$(DESTDIR)$(e_sysconfdir)/emailrelay.conf" "$(DESTDIR)$(e_sysconfdir)/emailrelay.conf.makeinstall"

# remove the .conf file if it has not been editied
uninstall-hook:
	if diff -q "$(DESTDIR)$(e_sysconfdir)/emailrelay.conf" "$(DESTDIR)$(e_sysconfdir)/emailrelay.conf.makeinstall" ; then rm "$(DESTDIR)$(e_sysconfdir)/emailrelay.conf" ; fi
	-rm "$(DESTDIR)$(e_sysconfdir)/emailrelay.conf.makeinstall"

else

# if there is no .conf file then copy it from .conf.template
install-data-hook:
	test -f "$(DESTDIR)$(e_sysconfdir)/emailrelay.conf" || cp "$(DESTDIR)$(e_sysconfdir)/emailrelay.conf.template" "$(DESTDIR)$(e_sysconfdir)/emailrelay.conf"

# remove the .conf file - the .conf.template has been deleted when this runs
uninstall-hook:
	-rm "$(DESTDIR)$(e_sysconfdir)/emailrelay.conf"

endif
