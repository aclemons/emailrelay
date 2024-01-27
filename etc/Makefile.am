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

if GCONFIG_PAM
 e_pam_DATA = emailrelay
else
 e_pam_DATA =
endif

EXTRA_DIST = \
	emailrelay.auth.in \
	emailrelay.conf.in \
	emailrelay.pam \
	emailrelay.service.in \
	emailrelay-fail2ban-jail \
	emailrelay-fail2ban-filter

noinst_DATA = \
	emailrelay.auth \
	emailrelay.conf

e_lib_DATA = \
	emailrelay.auth.in \
	emailrelay.conf.in

e_examples_DATA = \
	emailrelay-fail2ban-filter \
	emailrelay-fail2ban-jail

e_systemd_DATA = \
	emailrelay.service

emailrelay: emailrelay.pam
	cp $(top_srcdir)/etc/emailrelay.pam $@

do_sed = sed -e "s:__INIT_DIR__:$(e_initdir):g" -e "s:__RUN_DIR__:$(e_rundir):g" -e "s:__SYSCONF_DIR__:$(e_sysconfdir):g" -e "s:__SBIN_DIR__:$(sbindir):g"

emailrelay.service: emailrelay.service.in
	$(do_sed) < $(srcdir)/emailrelay.service.in > emailrelay.service

do_sed2 = sed -e "s:/var/spool/emailrelay:$(e_spooldir):g" -e "s:^.spool-dir .*:spool-dir $(e_spooldir):g" -e "s:/etc:$(e_sysconfdir):g" -e "s:/usr/local/sbin:$(sbindir):g"

emailrelay.conf: emailrelay.conf.in
	$(do_sed2) < $(srcdir)/emailrelay.conf.in > emailrelay.conf

emailrelay.auth: emailrelay.auth.in
	$(do_sed2) < $(srcdir)/emailrelay.auth.in > emailrelay.auth

CLEANFILES = \
	emailrelay \
	emailrelay.auth \
	emailrelay.conf \
	emailrelay.service

if GCONFIG_INSTALL_HOOK

# don't clobber user's auth and conf files -- disable with 'configure --disable-install-hook'

install-data-hook:
	-$(MKDIR_P) "$(DESTDIR)$(e_sysconfdir)/" || true
	if test -f "$(DESTDIR)$(e_sysconfdir)/emailrelay.conf" ; then : ; else $(INSTALL_DATA) "emailrelay.conf" "$(DESTDIR)$(e_sysconfdir)/" ; fi
	if test -f "$(DESTDIR)$(e_sysconfdir)/emailrelay.auth" ; then : ; else $(INSTALL_DATA) "emailrelay.auth" "$(DESTDIR)$(e_sysconfdir)/" ; fi

uninstall-hook:
	-cp "$(DESTDIR)$(e_sysconfdir)/emailrelay.conf" /tmp/emailrelay.conf.uninstall || true
	-rm "$(DESTDIR)$(e_sysconfdir)/emailrelay.conf" || true
	-rm "$(DESTDIR)$(e_sysconfdir)/emailrelay.auth" || true

endif
