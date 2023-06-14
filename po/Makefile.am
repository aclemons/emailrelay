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
#
# Makefile.am
#
# By default this makefile does nothing.
#
# Consider using 'gettextize' to convert the project files canonically, and
# after that this makefile should not be used.
#
# For testing do something like this:
#
#   $ apt-get install locales-all # or equivalent
#   $ ./configure.sh --with-gettext --disable-gui --without-doxygen
#   $ make
#   $ make install DESTDIR=`pwd`/install
#   $ cd po
#   $ make mo LANG=fr_FR
#   $ make inst DESTDIR=`pwd`/../install LANG=fr_FR
#   $ export LANGUAGE=fr
#   $ export LANG=fr_FR.utf8
#   $ ../install/usr/sbin/emailrelay --localedir `pwd`/../install/usr/share/locale --help
#
# Typically the catalogues are located at run-time using a path that is
# baked-in from the configure script's '--localedir' path, which is in turn
# based on its '--prefix' directory (eg. /usr/local). However, when
# installing with DESTDIR the emailrelay program needs the '--localedir'
# command-line option, as shown above, in order to find them (see
# src/main/run.cpp).
#

EXTRA_DIST = Makevars POTFILES.in

SUFFIXES = .pot .po .gmo .mo

.po.mo:
	msgfmt -o $*.mo $*.po

.PHONY: pot
pot: emailrelay.pot

.PHONY: po
po: emailrelay.$(LANG).po

emailrelay.$(LANG).po: emailrelay.pot
	msginit --no-wrap -o emailrelay.$(LANG).po -i emailrelay.pot
	perl -ani -e '$$x=$$_;$$y=$$x if $$x=~m/^msgid/;$$y=~s/^msgid "/msgstr "XX /;$$x=$$y if $$x =~ m/^msgstr/;print $$x' $@

.PHONY: mo
mo: emailrelay.$(LANG).mo

emailrelay.pot:
	@rm emailrelay.pot 2>/dev/null || true
	cd $(top_srcdir)/src ; xgettext -i --no-wrap --keyword=txt --keyword=tx --package-name=$(PACKAGE_NAME) --package-version=$(PACKAGE_VERSION) --c++ --force-po -o ../po/emailrelay.pot `find . -name *.cpp -o -name *.h`

emailrelay.$(LANG).mo: emailrelay.$(LANG).po
	msgfmt -o $@ $<

.PHONY: inst
inst:
	-$(mkdir_p) $(DESTDIR)$(localedir)/$(LANG)/LC_MESSAGES
	$(INSTALL_DATA) emailrelay.$(LANG).mo $(DESTDIR)$(localedir)/$(LANG)/LC_MESSAGES/emailrelay.mo
	cd $(DESTDIR)$(localedir) && ln -s $(LANG) `echo $(LANG) | sed 's/_.*//'`

.PHONY: uninst
uninst:
	rm -f $(DESTDIR)$(localedir)/$(LANG)/LC_MESSAGES/emailrelay.mo 2>/dev/null || true

CLEANFILES = emailrelay.pot *.po *.mo

