#
## Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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

EXTRA_DIST = emailrelay.sh_ txt2html.sh_ txt2mu.sh_ mu2html.sh_ mu2docbook.sh_ expand.sh_ emailrelay-notify.sh_ emailrelay-resubmit.sh_ emailrelay-deliver.sh_ emailrelay-process.sh_ emailrelay-runperl.js emailrelay-resubmit.js emailrelay-submit.sh_
work_scripts = txt2mu.sh mu2html.sh mu2docbook.sh expand.sh txt2html.sh emailrelay.sh
noinst_SCRIPTS = emailrelay-runperl.js emailrelay-resubmit.js $(work_scripts)
e_init_SCRIPTS = emailrelay
e_examples_DATA = emailrelay-process.sh emailrelay-notify.sh emailrelay-deliver.sh emailrelay-resubmit.sh emailrelay-submit.sh

CLEANFILES = $(e_init_SCRIPTS) $(e_examples_DATA) $(work_scripts)

SUFFIXES = .sh_ .sh

.sh_.sh:
	cat $(srcdir)/../bin/$*.sh_ | sed "s%__SPOOL_DIR__%${e_spooldir}%g;s%__SBIN_DIR__%${sbindir}%g;"'s/\[:digit:\]/0-9/g;s/\[:space:\]/ \\t/g' > $*.tmp && mv $*.tmp $*.sh 
	chmod ugo+x $*.sh

emailrelay: emailrelay.sh
	cp emailrelay.sh emailrelay
	chmod ugo+x emailrelay

install-data-local: install-e_examplesDATA install-e_initSCRIPTS
	chmod ugo+x $(DESTDIR)$(e_examplesdir)/emailrelay-notify.sh
	chmod ugo+x $(DESTDIR)$(e_examplesdir)/emailrelay-resubmit.sh
	chmod ugo+x $(DESTDIR)$(e_examplesdir)/emailrelay-deliver.sh
	chmod ugo+x $(DESTDIR)$(e_examplesdir)/emailrelay-submit.sh
	chmod ugo+x $(DESTDIR)$(e_examplesdir)/emailrelay-process.sh

uninstall-local:
	-rmdir $(DESTDIR)$(e_examplesdir) 2>/dev/null

