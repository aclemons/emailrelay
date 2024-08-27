#
## Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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

example_scripts_static = \
	emailrelay-bcc-check.pl \
	emailrelay-check-ipaddress.pl \
	emailrelay-dkim-signer.pl \
	emailrelay-ldap-verify.py \
	emailrelay-multicast.sh \
	emailrelay-rot13.pl \
	emailrelay-sendmail.pl

example_scripts_js = \
	emailrelay-check-ipaddress.js \
	emailrelay-set-message-id.js \
	emailrelay-set-from.js \
	emailrelay-set-from.pl \
	emailrelay-edit-content.js \
	emailrelay-edit-envelope.js \
	emailrelay-resubmit.js \
	emailrelay-service-install.js

example_scripts_in = \
	emailrelay-notify.sh.in \
	emailrelay-resubmit.sh.in \
	emailrelay-submit.sh.in

example_scripts_out = \
	emailrelay-notify.sh \
	emailrelay-resubmit.sh \
	emailrelay-submit.sh \
	emailrelay

init_script_in = \
	emailrelay.sh.in

init_script_out = \
	emailrelay

EXTRA_DIST = \
	$(example_scripts_static) \
	$(example_scripts_js) \
	$(example_scripts_in) \
	$(init_script_in)

e_examples_DATA = \
	$(example_scripts_js)

e_examples_SCRIPTS = \
	$(example_scripts_out) \
	$(example_scripts_static)

e_init_SCRIPTS = \
	$(init_script_out)

CLEANFILES = \
	$(example_scripts_out)

do_sed = sed -e "s%__SPOOL_DIR__%${e_spooldir}%g" -e "s%__SBIN_DIR__%${sbindir}%g" -e "s%__SYSCONF_DIR__%${e_sysconfdir}%g" -e "s%__LIB_DIR__%${e_libdir}%g" -e "s%__RUN_DIR__%${e_rundir}%g"

emailrelay-notify.sh: emailrelay-notify.sh.in
	$(do_sed) < $(srcdir)/emailrelay-notify.sh.in > emailrelay-notify.sh
	chmod +x emailrelay-notify.sh

emailrelay-resubmit.sh: emailrelay-resubmit.sh.in
	$(do_sed) < $(srcdir)/emailrelay-resubmit.sh.in > emailrelay-resubmit.sh
	chmod +x emailrelay-resubmit.sh

emailrelay-submit.sh: emailrelay-submit.sh.in
	$(do_sed) < $(srcdir)/emailrelay-submit.sh.in > emailrelay-submit.sh
	chmod +x emailrelay-submit.sh

emailrelay: emailrelay.sh.in
	$(do_sed) < $(srcdir)/emailrelay.sh.in > emailrelay
	chmod +x emailrelay

