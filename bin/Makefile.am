#
## Copyright (C) 2001-2003 Graeme Walker <graeme_walker@users.sourceforge.net>
## 
## This program is free software; you can redistribute it and/or
## modify it under the terms of the GNU General Public License
## as published by the Free Software Foundation; either
## version 2 of the License, or (at your option) any later
## version.
## 
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
## 
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
## 
#

EXTRA_DIST = emailrelay-doxygen-filter.sh_ emailrelay-test.sh_ emailrelay-soak.sh_ emailrelay.sh_ txt2html.sh_ txt2mu.sh_ mu2html.sh_ expand.sh_ emailrelay-notify.sh_ emailrelay-resubmit.sh_ emailrelay-deliver.sh_ emailrelay-process.sh_ emailrelay-runperl.js

noinst_SCRIPTS = emailrelay-doxygen-filter.sh emailrelay-test.sh emailrelay-soak.sh
e_init_SCRIPTS = emailrelay
e_examples_DATA = emailrelay-process.sh emailrelay-notify.sh emailrelay-deliver.sh emailrelay-resubmit.sh emailrelay-runperl.js
work_scripts = txt2mu.sh mu2html.sh expand.sh txt2html.sh emailrelay.sh

CLEANFILES = $(noinst_SCRIPTS) $(e_init_SCRIPTS) $(e_examples_DATA) $(work_scripts)

TESTS = emailrelay-test.sh

SUFFIXES = .sh_ .sh

.sh_.sh:
	cat $(srcdir)/../bin/$*.sh_ | sed "s%__SPOOL_DIR__%${e_spooldir}%g;s%__SBIN_DIR__%${e_sbindir}%g" > $*.tmp && mv $*.tmp $*.sh 
	chmod ugo+x $*.sh

emailrelay: emailrelay.sh
	cp emailrelay.sh emailrelay
	chmod ugo+x emailrelay

install-data-local: install-e_examplesDATA install-e_initSCRIPTS
	chmod ugo+x $(DESTDIR)$(e_examplesdir)/emailrelay-notify.sh
	chmod ugo+x $(DESTDIR)$(e_examplesdir)/emailrelay-resubmit.sh
	chmod ugo+x $(DESTDIR)$(e_examplesdir)/emailrelay-deliver.sh
	chmod ugo+x $(DESTDIR)$(e_examplesdir)/emailrelay-process.sh
	chmod ugo+x $(DESTDIR)$(e_examplesdir)/emailrelay-runperl.js

uninstall-local:
	-rmdir $(DESTDIR)$(e_examplesdir) 2>/dev/null

