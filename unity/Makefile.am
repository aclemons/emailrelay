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
# Optionally creates 'unity build' source files and builds them.
#
# Usage:
#     make programs
#
# Also:
#     make sources
#     make tidy
#
# Unity builds might build faster and can result in better-optimised
# executables. They can also make is easier to do static code
# analysis.
#

if GCONFIG_GUI

SOURCES_ = \
	emailrelay.cpp \
	emailrelay-passwd.cpp \
	emailrelay-submit.cpp \
	emailrelay-gui.cpp

PROGRAMS_ = \
	emailrelay \
	emailrelay-passwd \
	emailrelay-submit \
	emailrelay-gui

AM_CPPFLAGS = \
	-D "G_SBINDIR=$(sbindir)" \
	-D "G_SPOOLDIR=$(e_spooldir)" \
	-D "G_INITDIR=$(e_initdir)" \
	-D "G_SYSCONFDIR=$(e_sysconfdir)" \
	-D "G_ICONDIR=$(e_icondir)" \
	-D "G_LIBDIR=$(e_libdir)" \
	-D "G_MANDIR=$(mandir)" \
	-D "G_DOCDIR=$(e_docdir)" \
	-D "G_EXAMPLESDIR=$(e_examplesdir)" \
	-I . \
	$(GCONFIG_QT_CFLAGS)

MOC = moc_guidialog.cpp moc_guipage.cpp moc_guipages.cpp

else

SOURCES_ = \
	emailrelay.cpp \
	emailrelay-passwd.cpp \
	emailrelay-submit.cpp

PROGRAMS_ = \
	emailrelay \
	emailrelay-passwd \
	emailrelay-submit

AM_CPPFLAGS =

MOC =

endif

IFLAGS = \
	-I$(top_builddir)/src \
	-I$(top_srcdir)/src/glib \
	-I$(top_srcdir)/src/gssl \
	-I$(top_srcdir)/src/gnet \
	-I$(top_srcdir)/src/gauth \
	-I$(top_srcdir)/src/gpop \
	-I$(top_srcdir)/src/gstore \
	-I$(top_srcdir)/src/gsmtp \
	-I$(top_srcdir)/src/gfilters \
	-I$(top_srcdir)/src/gverifiers \
	-I$(top_srcdir)/src/win32 \
	-I$(top_srcdir)/src/gui \
	-I$(top_srcdir)/src/main

CDB = compile_commands.json

$(SOURCES_) $(CDB): Makefile Makefile.in
	-chmod +x $(top_srcdir)/libexec/make2unity || true
	$(top_srcdir)/libexec/make2unity --include=. --cdb --cdb-cxx="$(CXX)" --cdb-top=$(top_srcdir) --base=$(top_srcdir)/src $(SOURCES_)

.PHONY: programs
programs: $(PROGRAMS_)

.PHONY: sources
sources: $(SOURCES_)

.PHONY: cdb
cdb: $(CDB)

.PHONY: moc
moc: $(MOC)

emailrelay: emailrelay.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(IFLAGS) -DG_LIB_SMALL -o $@ $< $(LIBS) $(GCONFIG_TLS_LIBS)
	-strip $@

emailrelay-submit: emailrelay-submit.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(IFLAGS) -DG_LIB_SMALL -o $@ $< $(LIBS) $(GCONFIG_TLS_LIBS)
	-strip $@

emailrelay-passwd: emailrelay-passwd.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(IFLAGS) -DG_LIB_SMALL -o $@ $< $(LIBS) $(GCONFIG_TLS_LIBS)
	-strip $@

emailrelay-gui: emailrelay-gui.cpp $(MOC)
	$(CXX) $(AM_CPPFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(IFLAGS) -o $@ $< $(LIBS) $(GCONFIG_TLS_LIBS) $(GCONFIG_QT_LIBS)
	-strip $@

moc_guidialog.cpp: $(top_srcdir)/src/gui/guidialog.h
	$(GCONFIG_QT_MOC) $< -o $@

moc_guipage.cpp: $(top_srcdir)/src/gui/guipage.h
	$(GCONFIG_QT_MOC) $< -o $@

moc_guipages.cpp: $(top_srcdir)/src/gui/guipages.h
	$(GCONFIG_QT_MOC) $< -o $@

.PHONY: tidy
tidy: emailrelay.cpp $(CDB) .clang-tidy
	clang-tidy --header-filter=.* emailrelay.cpp

.PHONY: tidygui
tidygui: emailrelay.cpp $(CDB) $(MOC) .clang-tidy
	-@rm $(top_builddir)/src/gui/moc_guipage.cpp 2>/dev/null
	-@rm $(top_builddir)/src/gui/moc_guidialog.cpp 2>/dev/null
	-@rm $(top_builddir)/src/gui/moc_guipages.cpp 2>/dev/null
	clang-tidy --header-filter=src/ emailrelay-gui.cpp

.clang-tidy:
	cp $(top_srcdir)/src/.clang-tidy .

CLEANFILES = $(SOURCES_) $(PROGRAMS_) $(CDB) .clang-tidy $(MOC)

