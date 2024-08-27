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

txt_files_install=\
	readme.txt readme.md readme.rst \
	developer.txt developer.md developer.rst \
	reference.txt reference.md reference.rst \
	userguide.txt userguide.md userguide.rst \
	windows.txt windows.md windows.rst \
	changelog.txt changelog.md changelog.rst \
	doxygen.cfg.in

txt_files_noinstall=\
	.readthedocs.yaml \
	index.rst \
	conf.py

man_files_install=\
	emailrelay.1 \
	emailrelay-passwd.1 \
	emailrelay-submit.1

man_files_clean=\
	emailrelay.1.gz \
	emailrelay-passwd.1.gz \
	emailrelay-submit.1.gz

html_files_install=\
	index.html \
	emailrelay-man.html \
	readme.html \
	developer.html \
	reference.html \
	userguide.html \
	windows.html \
	changelog.html

html_files_noinstall=\
	man2html-missing.html \
	doxygen-missing.html

css_files_install=\
	emailrelay.css \
	emailrelay-doxygen.css

svg_files_noinstall=\
	authentication.svg \
	forwardto.svg \
	serverclient.svg \
	whatisit.svg \
	mailserver.svg \
	popbyname.svg

png_files_install=\
	authentication.png \
	forwardto.png \
	serverclient.png \
	whatisit.png \
	download-button.png \
	mailserver.png \
	popbyname.png

EXTRA_DIST = \
	$(man_files_install) \
	$(txt_files_install) \
	$(txt_files_noinstall) \
	$(css_files_install) \
	$(png_files_install) \
	$(svg_files_noinstall) \
	$(html_files_install) \
	$(html_files_noinstall)

noinst_SCRIPTS = .dox
man1_MANS = $(man_files_install)
e_doc_DATA = $(txt_files_install) $(html_files_install) $(css_files_install) $(png_files_install)
CLEANFILES = $(noinst_SCRIPTS) $(man_files_clean) emailrelay-man.html doxygen.out doxygen.cfg doxygen/search/* doxygen/*

SUFFIXES = .txt .html

run_doxygen=$(top_srcdir)/libexec/doxygen.sh

.dox:
	-@chmod +x $(run_doxygen)
	$(run_doxygen) "$(GCONFIG_HAVE_DOXYGEN)" "$(top_srcdir)" "$(top_builddir)" doxygen doxygen.cfg.in && touch .dox

emailrelay-man.html: emailrelay.1
	if test "$(GCONFIG_HAVE_MAN2HTML)" = "yes" ; then man2html -r -L. emailrelay.1 < /dev/null > emailrelay-man.html.tmp && mv emailrelay-man.html.tmp emailrelay-man.html ; grep -v '^Content-type:' < emailrelay-man.html | grep -v '^Time:' > emailrelay-man.html.tmp ; mv emailrelay-man.html.tmp emailrelay-man.html ; else cp man2html-missing.html emailrelay-man.html ; fi

emailrelay.1.gz : emailrelay.1
	if test -n "$(GZIP)" ; then $(GZIP) -c "$(top_srcdir)/doc/emailrelay.1" > emailrelay.1.gz ; fi

emailrelay-passwd.1.gz : emailrelay-passwd.1
	if test -n "$(GZIP)" ; then $(GZIP) -c "$(top_srcdir)/doc/emailrelay-passwd.1" > emailrelay-passwd.1.gz ; fi

emailrelay-submit.1.gz : emailrelay-submit.1
	if test -n "$(GZIP)" ; then $(GZIP) -c "$(top_srcdir)/doc/emailrelay-submit.1" > emailrelay-submit.1.gz ; fi

install-data-local: install-e_docDATA
	$(mkinstalldirs) "$(DESTDIR)$(e_docdir)/doxygen"
	if test -d doxygen ; then for file in doxygen/* ; do $(INSTALL) -m 644 "$$file" "$(DESTDIR)$(e_docdir)/$$file" || true ; done ; else true ; fi

uninstall-local:
	-find "$(DESTDIR)$(e_docdir)/doxygen" -type f | xargs rm -f
	-rmdir "$(DESTDIR)$(e_docdir)/doxygen" 2>/dev/null
	-rmdir "$(DESTDIR)$(e_docdir)" 2>/dev/null

.PHONY: rst
rst:
	ls -1 *.rst | grep -v index.rst | sed 's/.rst$$//' | xargs -i__ rst2html.py __.rst __.html

.PHONY: md
md:
	for f in *.md ; do markdown $$f > "`basename $$f .md`".html ; done

.PHONY: sphinx
sphinx:
	sphinx-build -b html . ./_build

.PHONY: website
website:
	@chmod +x $(top_srcdir)/libexec/make-website || true
	$(top_srcdir)/libexec/make-website `cat $(top_srcdir)/VERSION`

