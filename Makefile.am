#
## Copyright (C) 2001-2005 Graeme Walker <graeme_walker@users.sourceforge.net>
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
EXTRA_DIST = emailrelay.spec ChangeLog
SUBDIRS = src bin lib doc etc
e_doc_DATA = NEWS README changelog.gz
CLEANFILES = changelog.gz
changelog.gz: ChangeLog
	if test -n "$(GZIP)" ; then $(GZIP) -c $(top_srcdir)/ChangeLog > changelog.tmp && mv changelog.tmp changelog.gz ; fi
uninstall-local:
	-rmdir $(DESTDIR)$(e_docdir) 2>/dev/null
