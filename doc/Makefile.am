#
## Copyright (C) 2001-2004 Graeme Walker <graeme_walker@users.sourceforge.net>
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

stylesheet=emailrelay.css
txt_files=developer.txt reference.txt userguide.txt windows.txt 
man_files_in=emailrelay.1 emailrelay-passwd.1 emailrelay-poke.1 emailrelay-submit.1 
man_files_out=emailrelay.1.gz emailrelay-passwd.1.gz emailrelay-poke.1.gz emailrelay-submit.1.gz 
html_files_in=doxygen_header.html
html_files_thru=index.html emailrelay-man.html $(stylesheet)
html_files_out=readme.html developer.html reference.html userguide.html windows.html changelog.html
png_files=gsmtp-classes.png gnet-classes.png sequence-1.png sequence-2.png sequence-3.png sequence-4.png gnet-client.png gsmtp-serverprotocol.png gsmtp-scannerclient.png auth.png

EXTRA_DIST = $(man_files_in) $(txt_files) $(html_files_in) $(html_files_thru) $(png_files)
noinst_SCRIPTS = .dox
e_man1_DATA = $(man_files_out)
e_doc_DATA = $(txt_files) $(html_files_out) $(html_files_thru) $(png_files)
CLEANFILES = $(noinst_SCRIPTS) $(man_files_out) $(html_files_out) doxygen/*

SUFFIXES = .txt .html

filter=$(top_builddir)/bin/emailrelay-doxygen-filter.sh
filter_src=$(top_srcdir)/bin/emailrelay-doxygen-filter.sh_
converter=$(top_builddir)/bin/txt2html.sh
converter_src=$(top_srcdir)/bin/txt2html.sh_
converter_helper1=$(top_builddir)/bin/txt2mu.sh
converter_helper1_src=$(top_srcdir)/bin/txt2mu.sh_
converter_helper2=$(top_builddir)/bin/mu2html.sh
converter_helper2_src=$(top_srcdir)/bin/mu2html.sh_
converter_helper3=$(top_builddir)/bin/expand.sh
converter_helper3_src=$(top_srcdir)/bin/expand.sh_

.txt.html:
	$(converter) -a "$(AWK)" $(top_srcdir)/doc/$*.txt $(stylesheet) > $*.html

$(filter): $(filter_src)
	cp $(filter_src) $(filter)
	chmod ugo+x $(filter)

$(converter): $(converter_src) $(converter_helper1) $(converter_helper2) $(converter_helper3)
	cp $(converter_src) $(converter)
	chmod ugo+x $(converter)

$(converter_helper1): $(converter_helper1_src)
	cp $(converter_helper1_src) $(converter_helper1)
	chmod ugo+x $(converter_helper1)

$(converter_helper2): $(converter_helper2_src)
	cp $(converter_helper2_src) $(converter_helper2)
	chmod ugo+x $(converter_helper2)

$(converter_helper3): $(converter_helper3_src)
	cp $(converter_helper3_src) $(converter_helper3)
	chmod ugo+x $(converter_helper3)

.dox: $(filter)
	if test "$(HAVE_DOXYGEN)" = "yes" ; then cat $(top_srcdir)/src/main/doxygen.cfg | sed "s:__TOP_SRC__:$(top_srcdir):g" | sed "s:__TOP_BUILD__:$(top_builddir):g" | doxygen - && touch .dox ; else echo no doxygen ; fi

emailrelay-man.html: emailrelay.1
	if test "$(HAVE_MAN2HTML)" = "yes" ; then man2html emailrelay.1 < /dev/null > emailrelay-man.html.tmp && mv emailrelay-man.html.tmp emailrelay-man.html ; fi

developer.html reference.html userguide.html: $(converter)

readme.html: $(top_srcdir)/README $(converter)
	$(converter) -a "$(AWK)" $(top_srcdir)/README $(stylesheet) > readme.html

changelog.html: $(top_srcdir)/ChangeLog $(converter)
	$(converter) -a "$(AWK)" $(top_srcdir)/ChangeLog $(stylesheet) > changelog.html

emailrelay.1.gz : emailrelay.1
	if test -n "$(GZIP)" ; then $(GZIP) -c $(top_srcdir)/doc/emailrelay.1 > emailrelay.1.gz ; fi

emailrelay-passwd.1.gz : emailrelay-passwd.1
	if test -n "$(GZIP)" ; then $(GZIP) -c $(top_srcdir)/doc/emailrelay-passwd.1 > emailrelay-passwd.1.gz ; fi

emailrelay-submit.1.gz : emailrelay-submit.1
	if test -n "$(GZIP)" ; then $(GZIP) -c $(top_srcdir)/doc/emailrelay-submit.1 > emailrelay-submit.1.gz ; fi

emailrelay-poke.1.gz: emailrelay-poke.1
	if test -n "$(GZIP)" ; then $(GZIP) -c $(top_srcdir)/doc/emailrelay-poke.1 > emailrelay-poke.1.gz ; fi

install-data-local: install-e_docDATA install-e_man1DATA
	$(mkinstalldirs) $(DESTDIR)$(e_docdir)/doxygen
	if test "$(HAVE_DOXYGEN)" = "yes" ; then for file in doxygen/* ; do $(INSTALL) $$file $(DESTDIR)$(e_docdir)/$$file ; done ; fi

uninstall-local:
	-rm -f $(DESTDIR)$(e_docdir)/doxygen/* 2>/dev/null
	-rmdir $(DESTDIR)$(e_docdir)/doxygen 2>/dev/null
	-rmdir $(DESTDIR)$(e_docdir) 2>/dev/null

