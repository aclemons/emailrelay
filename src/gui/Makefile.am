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
# Builds emailrelay-gui.real.
#
# Note that the gui can be built with compiler options that are incompatible
# with the rest of the project because of glibsources.cpp.
#
# The following additional targets can be used for packaging:
#
# payload
#
#	Builds the install payload tree ("payload/") by collecting together
#	the result of running "make install" into a temporary directory.
#	Use "make MAKE_SETUP_DEBUG=-d payload" to stop the temporary
#	directory getting cleaned up.
#

MAC_SOURCES = guiaccess_mac.cpp guidir_mac.cpp guiboot_mac.cpp guilink_mac.cpp
WIN32_SOURCES = guiaccess_win32.cpp guidir_win32.cpp guiboot_win32.cpp guilink_win32.cpp emailrelay-gui.rc
UNIX_SOURCES = guiaccess_unix.cpp guidir_unix.cpp guiboot_unix.cpp guilink_unix.cpp

QT_SOURCES = \
	installer.cpp \
	guidialog.cpp \
	guidialog.h \
	guipage.cpp \
	guipage.h \
	guipages.cpp \
	guipages.h \
	gqt.h

LIB_SOURCES = \
	$(OS_SOURCES) \
	guidir.cpp \
	guidir.h \
	gcominit.h \
	glibsources.cpp \
	guiaccess.h \
	guiboot.h \
	guilink.h \
	guilegal.cpp \
	guilegal.h \
	installer.h \
	serverconfiguration.cpp \
	serverconfiguration.h

TS_FILES = emailrelay_tr.no_NO.ts
QM_IN_FILES = emailrelay.no.qm_in
QM_FILES = emailrelay.no.qm

emailrelay.no.qm: emailrelay.no.qm_in
	-$(AWK) '/^[0123456789]/ {for(i=1;i<NF;i++){printf("%c",$$i)}}' $(srcdir)/emailrelay.no.qm_in > emailrelay.no.qm
	@test -s $(QM_FILES)

if GCONFIG_WINDOWS
OS_SOURCES = $(WIN32_SOURCES)
OS_EXTRA_DIST = $(MAC_SOURCES) $(UNIX_SOURCES)
OS_NOINST =
else
if GCONFIG_MAC
OS_SOURCES = $(MAC_SOURCES)
OS_EXTRA_DIST = $(UNIX_SOURCES) $(WIN32_SOURCES)
OS_NOINST =
else
OS_SOURCES = $(UNIX_SOURCES)
OS_EXTRA_DIST = $(MAC_SOURCES) $(WIN32_SOURCES)
OS_NOINST =
endif
endif

EXTRA_DIST=\
	$(TS_FILES) \
	$(QM_IN_FILES) \
	emailrelay-gui.exe.manifest \
	emailrelay-gui.pro \
	messages.mc \
	$(OS_EXTRA_DIST)

# force symbol stripping on 'make install' -- see also make 'install-strip'
AM_INSTALL_PROGRAM_FLAGS=-s

MOC_OUTPUT=\
	moc_guidialog.cpp \
	moc_guipage.cpp \
	moc_guipages.cpp

CLEANFILES = $(MOC_OUTPUT) $(QM_FILES)
DISTCLEANFILES = ./.deps/moc*

AM_CPPFLAGS=\
	-I$(top_srcdir)/src/glib \
	-I$(top_srcdir)/src/gssl \
	-I$(top_srcdir)/src/gnet \
	-I$(top_srcdir)/src/main \
	-D "G_SBINDIR=$(sbindir)" \
	-D "G_SPOOLDIR=$(e_spooldir)" \
	-D "G_INITDIR=$(e_initdir)" \
	-D "G_SYSCONFDIR=$(e_sysconfdir)" \
	-D "G_ICONDIR=$(e_icondir)" \
	-D "G_LIBDIR=$(e_libdir)" \
	-D "G_MANDIR=$(mandir)" \
	-D "G_DOCDIR=$(e_docdir)" \
	-D "G_EXAMPLESDIR=$(e_examplesdir)" \
	$(GCONFIG_QT_CFLAGS)

if GCONFIG_GUI

moc_guidialog.cpp: $(srcdir)/guidialog.h
	$(GCONFIG_QT_MOC) $< -o $@

moc_guipage.cpp: $(srcdir)/guipage.h
	$(GCONFIG_QT_MOC) $< -o $@

moc_guipages.cpp: $(srcdir)/guipages.h
	$(GCONFIG_QT_MOC) $< -o $@

guidialog.o: moc_guidialog.cpp
guipage.o: moc_guipage.cpp
guipages.o: moc_guipages.cpp

noinst_SCRIPTS = $(OS_NOINST)
noinst_LIBRARIES = libguimain.a
libguimain_a_SOURCES = $(LIB_SOURCES)
e_tr_DATA = $(QM_FILES)

if GCONFIG_WINDOWS

sbin_PROGRAMS = emailrelay-gui

emailrelay_gui_SOURCES = guimain.cpp $(QT_SOURCES)
emailrelay_gui_DEPENDENCIES = libguimain.a
emailrelay_gui_LDADD = \
	libguimain.a \
	$(GCONFIG_QT_LIBS)

else

sbin_PROGRAMS = emailrelay-gui.real

emailrelay_gui_real_SOURCES = guimain.cpp $(QT_SOURCES)
emailrelay_gui_real_DEPENDENCIES = libguimain.a
emailrelay_gui_real_LDADD = \
	libguimain.a \
	$(GCONFIG_QT_LIBS)

endif

install-exec-hook:
	-mkdir -p "$(DESTDIR)$(sbindir)" 2>/dev/null
	@echo "#!/bin/sh" > "$(DESTDIR)$(sbindir)/emailrelay-gui"
	@echo exec $(DESTDIR)$(sbindir)/emailrelay-gui.real --qmdir=$(DESTDIR)$(e_trdir) \"$$\@\" >> "$(DESTDIR)$(sbindir)/emailrelay-gui"
	@echo "dir-install"=\"$(DESTDIR)/usr\" >> "$(DESTDIR)$(sbindir)/emailrelay-gui"
	@echo "dir-spool"=\"$(DESTDIR)$(e_spooldir)\" >> "$(DESTDIR)$(sbindir)/emailrelay-gui"
	@echo "dir-config"=\"$(DESTDIR)$(e_sysconfdir)\" >> "$(DESTDIR)$(sbindir)/emailrelay-gui"
	@chmod +x "$(DESTDIR)$(sbindir)/emailrelay-gui"

uninstall-hook:
	-rm -f "$(DESTDIR)$(sbindir)/emailrelay-gui"

clean-local:
	-rm -f payload

else

install-exec-hook:

uninstall-hook:

endif

payload: $(top_srcdir)/libexec/make-setup.sh
	-mkdir payload 2>/dev/null
	-@chmod +x $(top_srcdir)/libexec/make-setup.sh
	$(top_srcdir)/libexec/make-setup.sh $(MAKE_SETUP_DEBUG) $@

.PHONY: lrelease
lrelease:
	"$(QT_LRELEASE)" emailrelay_tr.no_NO.ts -qm emailrelay.no.qm

