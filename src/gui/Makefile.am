#
## Copyright (C) 2001-2008 Graeme Walker <graeme_walker@users.sourceforge.net>
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

BUNDLE=E-MailRelay-Configure
SETUP_BUNDLE=E-MailRelay-Install
IMAGE=E-MailRelay.dmg

if MAC
MAC_EXTRA_DIST = dir_unix.cpp boot_unix.cpp glink_unix.cpp emailrelay-icon.icns
MAC_SOURCES = dir_mac.cpp boot_mac.cpp glink_mac.cpp
MAC_NOINST = $(BUNDLE).app
MAC_PROGS = emailrelay-start-gui emailrelay-unpack
MAKE_SETUP_STUB = payload-stub
else
MAC_EXTRA_DIST = dir_mac.cpp boot_mac.cpp glink_mac.cpp guistart.cpp emailrelay-icon.icns
MAC_SOURCES = dir_unix.cpp boot_unix.cpp glink_unix.cpp
MAC_PROGS =
MAC_NOINST =
MAKE_SETUP_STUB = emailrelay-gui.real
endif

EXTRA_DIST=\
	boot_win32.cpp \
	dir_win32.cpp \
	emailrelay-icon.png \
	gcominit.h \
	glink_win32.cpp \
	mingw.mak \
	mock \
	pack.cpp \
	run.c \
	$(MAC_EXTRA_DIST)

if GUI

# force symbol stripping on 'make install' -- see also make 'install-strip'
AM_INSTALL_PROGRAM_FLAGS=-s

MOC_OUTPUT=\
	moc_gdialog.cpp \
	moc_gpage.cpp \
	moc_pages.cpp

MOC_OBJECTS=\
	moc_gdialog.o \
	moc_gpage.o \
	moc_pages.o

moc_gdialog.cpp: $(srcdir)/gdialog.h $(mock)
	$(MOC) $< -o $@

moc_gpage.cpp: $(srcdir)/gpage.h $(mock)
	$(MOC) $< -o $@

moc_pages.cpp: $(srcdir)/pages.h $(mock)
	$(MOC) $< -o $@

CLEANFILES = $(MOC_OUTPUT) $(mock)

INCLUDES=\
	-I$(top_srcdir)/lib/$(COMPILER_VERSION) \
	-I$(top_srcdir)/src/glib \
	-I$(top_srcdir)/src/gnet \
	-I$(top_srcdir)/src/gsmtp \
	-I$(top_srcdir)/src/gpop \
	-I$(top_srcdir)/src/main \
	-DG_SBINDIR=\"$(sbindir)\" \
	-DG_SPOOLDIR=\"$(e_spooldir)\" \
	-DG_INITDIR=\"$(e_initdir)\" \
	-DG_SYSCONFDIR=\"$(e_sysconfdir)\" \
	-DG_LIBEXECDIR=\"$(e_libexecdir)\" \
	-DG_MANDIR=\"$(mandir)\" \
	-DG_DOCDIR=\"$(e_docdir)\" \
	-DG_EXAMPLESDIR=\"$(e_examplesdir)\"

AM_CPPFLAGS=$(QT_CFLAGS)

sbin_PROGRAMS = emailrelay-gui.real $(MAC_PROGS)
noinst_SCRIPTS = mock $(MAC_NOINST)
noinst_PROGRAMS = pack emailrelay-unpack

emailrelay_gui_real_SOURCES = \
	$(MAC_SOURCES) \
	boot.h \
	dir.cpp \
	dir.h \
	gdialog.cpp \
	gdialog.h \
	glink.h \
	gpage.cpp \
	gpage.h \
	guimain.cpp \
	gunpack.cpp \
	gunpack.h \
	installer.cpp \
	installer.h \
	legal.cpp \
	legal.h \
	pages.cpp \
	pages.h \
	qt.h \
	state.cpp \
	state.h \
	unpack.c \
	unpack.h

emailrelay_gui_real_DEPENDENCIES = $(MOC_OBJECTS)

emailrelay_gui_real_LDADD = \
	$(MOC_OBJECTS) \
	$(top_builddir)/src/glib/libglib.a \
	$(QT_LIBS)

pack_SOURCES = pack.cpp
pack_LDADD = $(top_builddir)/src/glib/libglib.a

emailrelay_unpack_SOURCES = unpackmain.c unpack.c
emailrelay_unpack_LDADD =

emailrelay_start_gui_SOURCES = guistart.cpp
emailrelay_start_gui_LDADD =

install-exec-hook:
	@echo "#!/bin/sh" > "$(DESTDIR)$(sbindir)/emailrelay-gui"
	@echo DIR_SPOOL=\"$(DESTDIR)$(e_spooldir)\" >> "$(DESTDIR)$(sbindir)/emailrelay-gui"
	@echo DIR_CONFIG=\"$(DESTDIR)$(e_sysconfdir)\" >> "$(DESTDIR)$(sbindir)/emailrelay-gui"
	@echo exec $(DESTDIR)$(sbindir)/emailrelay-gui.real \"$$\@\" >> "$(DESTDIR)$(sbindir)/emailrelay-gui"
	@chmod +x "$(DESTDIR)$(sbindir)/emailrelay-gui"
	-if test -d "$(BUNDLE).app" ; then cp -R "$(BUNDLE).app" "$(DESTDIR)$(sbindir)" ; fi

uninstall-hook:
	-rm "$(DESTDIR)$(sbindir)/emailrelay-gui"
	-rm -rf "$(DESTDIR)$(sbindir)/$(BUNDLE).app"

clean-local:
	-rm -rf "$(BUNDLE).app"
	-rm -rf "$(SETUP_BUNDLE).app"
	-rm -f emailrelay-setup payload-stub payload
	-rm -f "$(IMAGE)"
	-rm -f "emailrelay-setup.state"

else

install-exec-hook:

uninstall-hook:

endif

$(BUNDLE).app: emailrelay-start-gui
	../../bin/make-bundle.sh -f "$(BUNDLE)" emailrelay-start-gui "$(top_srcdir)/src/gui/emailrelay-icon.icns"

.PHONY: bundle
bundle: $(BUNDLE).app

payload-stub:
	-@echo '#!/bin/sh' > payload-stub
	-@echo 'exit 23' >> payload-stub

emailrelay-setup: pack $(MAKE_SETUP_STUB) ../../bin/make-setup.sh
	../../bin/make-setup.sh $(MAKE_SETUP_DEBUG) $@ $(MAKE_SETUP_STUB) pack "$(top_srcdir)/src/gui/emailrelay-icon.png"
	@chmod +x $@

payload: pack payload-stub ../../bin/make-setup.sh
	../../bin/make-setup.sh $(MAKE_SETUP_DEBUG) $@ payload-stub pack "$(top_srcdir)/src/gui/emailrelay-icon.png"

$(SETUP_BUNDLE).app: emailrelay-gui.real payload ../../bin/make-bundle.sh
	../../bin/make-bundle.sh -f "$(SETUP_BUNDLE)" emailrelay-gui.real "$(top_srcdir)/src/gui/emailrelay-icon.icns"
	mv "$(SETUP_BUNDLE).app/Contents/MacOS/E-MailRelay-Install" "$(SETUP_BUNDLE).app/Contents/emailrelay-gui.real"
	( cd "$(SETUP_BUNDLE).app/Contents/MacOS" && ln -s ../emailrelay-gui.real E-MailRelay-Install )
	../../bin/make-qt-enabled.sh -f "$(SETUP_BUNDLE).app/Contents/emailrelay-gui.real" "Frameworks"
	cp payload "$(SETUP_BUNDLE).app/Contents/"

.PHONY: setup-bundle
setup-bundle: $(SETUP_BUNDLE).app

$(IMAGE): $(SETUP_BUNDLE).app emailrelay-unpack
	-@rm -f "$(SETUP_BUNDLE).app/Contents/MacOS/$(SETUP_BUNDLE).state"
	cp emailrelay-unpack "$(SETUP_BUNDLE).app/Contents/"
	hdiutil create -srcfolder "$(SETUP_BUNDLE).app" "$(IMAGE)" && hdiutil internet-enable -yes "$(IMAGE)"

.PHONY: image
image: $(IMAGE)

