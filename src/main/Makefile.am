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

if ADMIN
ADMIN_EXTRA_DIST = admin_disabled.cpp
ADMIN_SOURCES = admin_enabled.cpp
else
ADMIN_EXTRA_DIST = admin_enabled.cpp
ADMIN_SOURCES = admin_disabled.cpp
endif

if SMALL_CONFIG
CONFIG_EXTRA_DIST = commandline_full.cpp
CONFIG_SOURCES = commandline_simple.cpp
else
CONFIG_EXTRA_DIST = commandline_simple.cpp
CONFIG_SOURCES = commandline_full.cpp
endif

BUNDLE=E-MailRelay-Start
if MAC
 MAC_PROGS = emailrelay-start
 MAC_EXTRA_DIST =
 MAC_NOINST = $(BUNDLE).app
else
 MAC_PROGS =
 MAC_EXTRA_DIST = start.cpp
 MAC_NOINST =
endif

# force symbol stripping on 'make install' -- see also make 'install-strip'
AM_INSTALL_PROGRAM_FLAGS=-s

EXTRA_DIST=\
	$(ADMIN_EXTRA_DIST) \
	$(CONFIG_EXTRA_DIST) \
	$(MAC_EXTRA_DIST) \
	doxygen.h \
	emailrelay.rc \
	icon1.ico \
	icon2.ico \
	icon3.ico \
	icon4.ico \
	messages.mc \
	news.cpp \
	news.h \
	resource.h \
	service_install.cpp \
	service_install.h \
	service_remove.cpp \
	service_remove.h \
	service_wrapper.cpp \
	winapp.cpp \
	winapp.h \
	winform.cpp \
	winform.h \
	winmain.cpp \
	winmenu.cpp \
	winmenu.h \
	fakemc.c \
	mingw.mak

INCLUDES = -I$(top_srcdir)/lib/$(COMPILER_VERSION) -I$(top_srcdir)/src/glib -I$(top_srcdir)/src/gssl -I$(top_srcdir)/src/gnet -I$(top_srcdir)/src/gauth -I$(top_srcdir)/src/gsmtp -I$(top_srcdir)/src/gpop -DG_SPOOLDIR=\"$(e_spooldir)\" -DG_CAPABILITIES=\"$(G_CAPABILITIES)\"

sbin_PROGRAMS = emailrelay emailrelay-submit emailrelay-passwd $(MAC_PROGS)
e_libexec_PROGRAMS = emailrelay-poke emailrelay-filter-copy
e_spool_DATA = 
noinst_SCRIPTS = $(MAC_NOINST)

emailrelay_SOURCES = \
	$(ADMIN_SOURCES) \
	$(CONFIG_SOURCES) \
	admin.h \
	commandline.h \
	configuration.h \
	configuration.cpp \
	legal.cpp \
	legal.h \
	main.cpp \
	output.cpp \
	output.h \
	run.cpp \
	run.h

COMMON_LDADD = \
	$(top_builddir)/src/glib/libglib.a

emailrelay_LDADD = \
	$(top_builddir)/src/gpop/libgpop.a \
	$(top_builddir)/src/gsmtp/libgsmtp.a \
	$(top_builddir)/src/gauth/libgauth.a \
	$(top_builddir)/src/gnet/libgnet.a \
	$(top_builddir)/src/gssl/libgssl.a \
	$(COMMON_LDADD) \
	$(STATIC_START) $(PAM_LIBS) $(SSL_LIBS) $(STATIC_END)

emailrelay_poke_SOURCES = poke.c
emailrelay_poke_LDADD =

emailrelay_passwd_SOURCES = passwd.cpp legal.cpp legal.h
emailrelay_passwd_LDADD = \
	$(COMMON_LDADD)

emailrelay_submit_SOURCES = submit.cpp legal.cpp legal.h
emailrelay_submit_LDADD = \
	$(top_builddir)/src/gsmtp/libgsmtp.a \
	$(top_builddir)/src/gauth/libgauth.a \
	$(top_builddir)/src/gnet/libgnet.a \
	$(COMMON_LDADD)

emailrelay_filter_copy_SOURCES = filter_copy.cpp filter.cpp filter.h legal.cpp legal.h
emailrelay_filter_copy_LDADD = \
	$(COMMON_LDADD)

emailrelay_start_SOURCES = start.cpp
emailrelay_start_LDADD =

install-data-local: install-sbinPROGRAMS install-e_spoolDATA install-e_libexecPROGRAMS 
	-mkdir -p "$(DESTDIR)$(e_spooldir)"
	-chgrp daemon "$(DESTDIR)$(e_spooldir)"
	-chmod 775 "$(DESTDIR)$(e_spooldir)"
	-chgrp daemon "$(DESTDIR)$(sbindir)/emailrelay-submit"
	-chmod g+s "$(DESTDIR)$(sbindir)/emailrelay-submit"

install-exec-local:
	-if test -d "$(BUNDLE).app" ; then cp -R "$(BUNDLE).app" "$(DESTDIR)$(sbindir)" ; fi

uninstall-local:
	-rmdir "$(DESTDIR)$(e_spooldir)" 2>/dev/null
	-rmdir "$(DESTDIR)$(e_libexecdir)" 2>/dev/null
	-rm -rf "$(DESTDIR)$(sbindir)/$(BUNDLE).app"

clean-local:
	rm -rf "$(BUNDLE).app"

$(BUNDLE).app: emailrelay-start $(sbin_PROGRAMS) $(e_libexec_PROGRAMS)
	../../bin/make-bundle.sh "$(BUNDLE)" emailrelay-start "$(top_srcdir)/src/gui/emailrelay-icon.icns"

.PHONY: bundle
bundle: emailrelay-start $(sbin_PROGRAMS) $(e_libexec_PROGRAMS)
	../../bin/make-bundle.sh -f "$(BUNDLE)" emailrelay-start "$(top_srcdir)/src/gui/emailrelay-icon.icns"

