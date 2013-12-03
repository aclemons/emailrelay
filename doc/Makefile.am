#
## Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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

txt_files=developer.txt reference.txt userguide.txt windows.txt
man_files_in=emailrelay.1 emailrelay-passwd.1 emailrelay-poke.1 emailrelay-submit.1 emailrelay-filter-copy.1
man_files_out=emailrelay.1.gz emailrelay-passwd.1.gz emailrelay-poke.1.gz emailrelay-submit.1.gz emailrelay-filter-copy.1.gz
html_files_in=doxygen-header.html doxygen-missing.html doxygen.cfg
html_files_thru=index.html emailrelay-man.html
html_files_out=readme.html developer.html reference.html userguide.html windows.html changelog.html
docbook_files_out=emailrelay.docbook
css_files_in=\
	emailrelay.css_ \
	emailrelay-doxygen.css_
css_files_out=\
	emailrelay.css \
	emailrelay-doxygen.css
png_files_thru=\
	valid-html401.png \
	diagram-1.png \
	diagram-2.png
png_files_in=\
	gsmtp-classes.png_ \
	gnet-classes.png_ \
	sequence-3.png_ \
	gnet-client.png_ \
	gsmtp-serverprotocol.png_ \
	auth.png_
png_files_out=\
	gsmtp-classes.png \
	gnet-classes.png \
	sequence-3.png \
	gnet-client.png \
	gsmtp-serverprotocol.png \
	auth.png

EXTRA_DIST = $(man_files_in) $(txt_files) $(html_files_in) $(css_files_in) $(png_files_in) $(png_files_thru) $(html_files_thru) mingw.mak
noinst_SCRIPTS = .dox .docbook
man1_MANS = $(man_files_in)
e_doc_DATA = $(txt_files) $(html_files_out) $(html_files_thru) $(css_files_out) $(png_files_out) $(png_files_thru) $(docbook_files_out)
CLEANFILES = $(noinst_SCRIPTS) $(man_files_out) $(html_files_out) $(docbook_files_out) $(png_files_out) $(css_files_out) Doxyfile *.db *.mu doxygen/* docbook/*

SUFFIXES = .txt .html .db .mu

converter_html=$(top_builddir)/bin/txt2html.sh
converter_txt2mu=$(top_builddir)/bin/txt2mu.sh
converter_mu2html=$(top_builddir)/bin/mu2html.sh
converter_expand=$(top_builddir)/bin/expand.sh
converter_mu2docbook=$(top_builddir)/bin/mu2docbook.sh
run_doxygen=$(top_builddir)/bin/doxygen.sh
run_doxygen_fixup=$(top_builddir)/bin/doxygen_fixup.pl
css=emailrelay.css

.txt.html:
	$(converter_html) -a "$(AWK)" "$(top_srcdir)/doc/$*.txt" "$(css)" > "$*.html"

.txt.mu:
	$(converter_txt2mu) -a "$(AWK)" "$(top_srcdir)/doc/$*.txt" > "$*.mu"

.mu.db:
	$(converter_mu2docbook) -a "$(AWK)" -x < "$(top_builddir)/doc/$*.mu" > "$*.db"

.css_.css:
	cp "$(top_srcdir)/doc/$*.css_" $*.css

.png_.png:
	cp "$(top_srcdir)/doc/$*.png_" $*.png

head.db: $(converter_mu2docbook)
	$(converter_mu2docbook) -a "$(AWK)" --head "E-MailRelay" > head.db.tmp && mv -f head.db.tmp head.db

tail.db: $(converter_mu2docbook)
	$(converter_mu2docbook) -a "$(AWK)" --tail > tail.db.tmp && mv -f tail.db.tmp tail.db

emailrelay.docbook: head.db tail.db userguide.db reference.db developer.db
	cat head.db userguide.db reference.db developer.db tail.db > emailrelay.docbook

.docbook: emailrelay.docbook $(png_files_out)
	-xmlto -o docbook html emailrelay.docbook && touch .docbook && cp "$(top_srcdir)/doc/"*.png docbook/

.dox:
	$(run_doxygen) "$(HAVE_DOXYGEN)" "$(top_srcdir)" "$(top_builddir)" Doxyfile && touch .dox
	-$(run_doxygen_fixup) "$(top_builddir)/doc/doxygen"
	-cp "$(top_srcdir)/doc/emailrelay-doxygen.css_" "$(top_builddir)/doc/doxygen/emailrelay-doxygen.css"

emailrelay-man.html: emailrelay.1
	if test "$(HAVE_MAN2HTML)" = "yes" ; then man2html -r -L. emailrelay.1 < /dev/null > emailrelay-man.html.tmp && mv emailrelay-man.html.tmp emailrelay-man.html ; grep -v '^Content-type:' < emailrelay-man.html | grep -v '^Time:' > emailrelay-man.html.tmp ; mv emailrelay-man.html.tmp emailrelay-man.html ; fi

developer.html reference.html userguide.html: $(converter_html)

developer.mu reference.mu userguide.mu: $(converter_txt2mu)

readme.html: $(top_srcdir)/README $(converter_html)
	$(converter_html) -a "$(AWK)" "$(top_srcdir)/README" "$(css)" > readme.html

changelog.html: $(top_srcdir)/ChangeLog $(converter_html)
	$(converter_html) -a "$(AWK)" "$(top_srcdir)/ChangeLog" "$(css)" > changelog.html

emailrelay.1.gz : emailrelay.1
	if test -n "$(GZIP)" ; then $(GZIP) -c "$(top_srcdir)/doc/emailrelay.1" > emailrelay.1.gz ; fi

emailrelay-passwd.1.gz : emailrelay-passwd.1
	if test -n "$(GZIP)" ; then $(GZIP) -c "$(top_srcdir)/doc/emailrelay-passwd.1" > emailrelay-passwd.1.gz ; fi

emailrelay-submit.1.gz : emailrelay-submit.1
	if test -n "$(GZIP)" ; then $(GZIP) -c "$(top_srcdir)/doc/emailrelay-submit.1" > emailrelay-submit.1.gz ; fi

emailrelay-filter-copy.1.gz : emailrelay-filter-copy.1
	if test -n "$(GZIP)" ; then $(GZIP) -c "$(top_srcdir)/doc/emailrelay-filter-copy.1" > emailrelay-filter-copy.1.gz ; fi

emailrelay-poke.1.gz: emailrelay-poke.1
	if test -n "$(GZIP)" ; then $(GZIP) -c "$(top_srcdir)/doc/emailrelay-poke.1" > emailrelay-poke.1.gz ; fi

install-data-local: install-e_docDATA
	$(mkinstalldirs) "$(DESTDIR)$(e_docdir)/doxygen"
	for file in doxygen/* ; do $(INSTALL) -m 644 "$$file" "$(DESTDIR)$(e_docdir)/$$file" ; done

uninstall-local:
	-find "$(DESTDIR)$(e_docdir)/doxygen" -type f | xargs rm -f
	-rmdir "$(DESTDIR)$(e_docdir)/doxygen" 2>/dev/null
	-rmdir "$(DESTDIR)$(e_docdir)" 2>/dev/null

