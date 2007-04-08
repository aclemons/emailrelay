#
## Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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
docbook_files_out=emailrelay.docbook
png_files=gsmtp-classes.png gnet-classes.png sequence-1.png sequence-2.png sequence-3.png sequence-4.png gnet-client.png gsmtp-serverprotocol.png gsmtp-scannerclient.png auth.png

EXTRA_DIST = $(man_files_in) $(txt_files) $(html_files_in) $(html_files_thru) $(png_files)
noinst_SCRIPTS = .dox .docbook
man1_MANS = $(man_files_in)
e_doc_DATA = $(txt_files) $(html_files_out) $(html_files_thru) $(png_files) $(docbook_files_out)
CLEANFILES = $(noinst_SCRIPTS) $(man_files_out) $(html_files_out) $(docbook_files_out) doxygen/* *.db *.mu docbook/*

SUFFIXES = .txt .html .db .mu

converter_html=$(top_builddir)/bin/txt2html.sh
converter_html_src=$(top_srcdir)/bin/txt2html.sh_
converter_txt2mu=$(top_builddir)/bin/txt2mu.sh
converter_txt2mu_src=$(top_srcdir)/bin/txt2mu.sh_
converter_mu2html=$(top_builddir)/bin/mu2html.sh
converter_mu2html_src=$(top_srcdir)/bin/mu2html.sh_
converter_expand=$(top_builddir)/bin/expand.sh
converter_expand_src=$(top_srcdir)/bin/expand.sh_
converter_mu2docbook=$(top_builddir)/bin/mu2docbook.sh
converter_mu2docbook_src=$(top_srcdir)/bin/mu2docbook.sh_

.txt.html:
	$(converter_html) -a "$(AWK)" $(top_srcdir)/doc/$*.txt $(stylesheet) > $*.html

.txt.mu:
	$(converter_txt2mu) -a "$(AWK)" $(top_srcdir)/doc/$*.txt > $*.mu

.mu.db:
	$(converter_mu2docbook) -a "$(AWK)" -x < $(top_builddir)/doc/$*.mu > $*.db

$(converter_html): $(converter_html_src) $(converter_txt2mu) $(converter_mu2html) $(converter_expand) 
	cp $(converter_html_src) $(converter_html)
	chmod ugo+x $(converter_html)

head.db: $(converter_mu2docbook)
	$(converter_mu2docbook) --head "E-MailRelay" > head.db.tmp && mv -f head.db.tmp head.db

tail.db: $(converter_mu2docbook)
	$(converter_mu2docbook) --tail > tail.db.tmp && mv -f tail.db.tmp tail.db

emailrelay.docbook: head.db tail.db userguide.db reference.db developer.db
	cat head.db userguide.db reference.db developer.db tail.db > emailrelay.docbook

.docbook: emailrelay.docbook
	-xmlto -o docbook html emailrelay.docbook && touch .docbook && cp *.png docbook/

$(converter_docbook): $(converter_docbook_src) $(converter_mu2docbook)
	cp $(converter_docbook_src) $(converter_db)
	chmod ugo+x $(converter_docbook)

$(converter_txt2mu): $(converter_txt2mu_src)
	cp $(converter_txt2mu_src) $(converter_txt2mu)
	chmod ugo+x $(converter_txt2mu)

$(converter_mu2html): $(converter_mu2html_src)
	cp $(converter_mu2html_src) $(converter_mu2html)
	chmod ugo+x $(converter_mu2html)

$(converter_expand): $(converter_expand_src)
	cp $(converter_expand_src) $(converter_expand)
	chmod ugo+x $(converter_expand)

$(converter_mu2docbook): $(converter_mu2docbook_src)
	cp $(converter_mu2docbook_src) $(converter_mu2docbook)
	chmod ugo+x $(converter_mu2docbook)

.dox:
	if test "$(HAVE_DOXYGEN)" = "yes" ; then cat $(top_srcdir)/src/main/doxygen.cfg | sed "s:__TOP_SRC__:$(top_srcdir):g" | sed "s:__TOP_BUILD__:$(top_builddir):g" | doxygen - && touch .dox ; else echo no doxygen ; fi

emailrelay-man.html: emailrelay.1
	if test "$(HAVE_MAN2HTML)" = "yes" ; then man2html emailrelay.1 < /dev/null > emailrelay-man.html.tmp && mv emailrelay-man.html.tmp emailrelay-man.html ; fi

developer.html reference.html userguide.html: $(converter_html)

developer.mu reference.mu userguide.mu: $(converter_txt2mu)

readme.html: $(top_srcdir)/README $(converter_html)
	$(converter_html) -a "$(AWK)" $(top_srcdir)/README $(stylesheet) > readme.html

changelog.html: $(top_srcdir)/ChangeLog $(converter_html)
	$(converter_html) -a "$(AWK)" $(top_srcdir)/ChangeLog $(stylesheet) > changelog.html

emailrelay.1.gz : emailrelay.1
	if test -n "$(GZIP)" ; then $(GZIP) -c $(top_srcdir)/doc/emailrelay.1 > emailrelay.1.gz ; fi

emailrelay-passwd.1.gz : emailrelay-passwd.1
	if test -n "$(GZIP)" ; then $(GZIP) -c $(top_srcdir)/doc/emailrelay-passwd.1 > emailrelay-passwd.1.gz ; fi

emailrelay-submit.1.gz : emailrelay-submit.1
	if test -n "$(GZIP)" ; then $(GZIP) -c $(top_srcdir)/doc/emailrelay-submit.1 > emailrelay-submit.1.gz ; fi

emailrelay-poke.1.gz: emailrelay-poke.1
	if test -n "$(GZIP)" ; then $(GZIP) -c $(top_srcdir)/doc/emailrelay-poke.1 > emailrelay-poke.1.gz ; fi

install-data-local: install-e_docDATA
	$(mkinstalldirs) $(DESTDIR)$(e_docdir)/doxygen
	if test "$(HAVE_DOXYGEN)" = "yes" ; then for file in doxygen/* ; do $(INSTALL) $$file $(DESTDIR)$(e_docdir)/$$file ; done ; fi

uninstall-local:
	-rm -f $(DESTDIR)$(e_docdir)/doxygen/* 2>/dev/null
	-rmdir $(DESTDIR)$(e_docdir)/doxygen 2>/dev/null
	-rmdir $(DESTDIR)$(e_docdir) 2>/dev/null

