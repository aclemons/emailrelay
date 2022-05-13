#
## Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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

SUBDIRS = icon
sbin_PROGRAMS = emailrelay emailrelay-submit emailrelay-passwd $(MAC_PROGS) $(WINDOWS_PROGS)
e_libexec_PROGRAMS = emailrelay-filter-copy
e_spool_DATA =
noinst_SCRIPTS = $(MAC_NOINST)
noinst_LIBRARIES = libmain.a

# force symbol stripping on 'make install' -- see also make 'install-strip'
AM_INSTALL_PROGRAM_FLAGS = -s

AM_CPPFLAGS = \
	-I$(top_srcdir)/src/glib \
	-I$(top_srcdir)/src/gssl \
	-I$(top_srcdir)/src/gnet \
	-I$(top_srcdir)/src/win32 \
	-I$(top_srcdir)/src/gauth \
	-I$(top_srcdir)/src/gsmtp \
	-I$(top_srcdir)/src/gpop \
	-D "G_SPOOLDIR=$(e_spooldir)" \
	-D "G_LOCALEDIR=$(localedir)"

WINDOWS_MAIN_SOURCES = \
	winmain.cpp \
	winapp.cpp \
	winapp.h \
	winform.cpp \
	winform.h \
	winmenu.cpp \
	winmenu.h \
	licence.cpp \
	licence.h \
	news.cpp \
	news.h

TEXTMODE_MAIN_SOURCES = \
	main.cpp

OTHER_SOURCES = \
	commandline.h \
	commandline.cpp \
	configuration.h \
	configuration.cpp \
	legal.cpp \
	legal.h \
	output.cpp \
	output.h \
	run.cpp \
	run.h

WINDOWS_SERVICEWRAPPER_SOURCES = \
	servicewrapper.cpp

WINDOWS_LIBMAIN_SOURCES = \
	serviceimp_win32.cpp \
	servicecontrol_win32.cpp

UNIX_LIBMAIN_SOURCES = \
	serviceimp_none.cpp \
	servicecontrol_unix.cpp

LIBMAIN_SOURCES = \
	options.cpp \
	options.h \
	serviceimp.h \
	servicecontrol.h

MISC_EXTRA_DIST = \
	doxygen.h \
	emailrelay.rc \
	emailrelay.exe.manifest \
	emailrelay-service.exe.manifest \
	fakemc.c \
	messages.mc \
	resource.h

if GCONFIG_MAC
 MAC_PROGS = emailrelay-start
 MAC_EXTRA_DIST =
 MAC_NOINST =
else
 MAC_PROGS =
 MAC_EXTRA_DIST = start.cpp
 MAC_NOINST =
endif

if GCONFIG_WINDOWS
 WINDOWS_PROGS = emailrelay-service emailrelay-textmode
else
 WINDOWS_PROGS =
endif

if GCONFIG_WINDOWS
 EXTRA_DIST=\
	$(MAC_EXTRA_DIST) \
	$(MISC_EXTRA_DIST) \
	$(UNIX_LIBMAIN_SOURCES)
else
 EXTRA_DIST=\
	$(MAC_EXTRA_DIST) \
	$(MISC_EXTRA_DIST) \
	$(WINDOWS_MAIN_SOURCES) \
	$(WINDOWS_SERVICEWRAPPER_SOURCES) \
	$(WINDOWS_LIBMAIN_SOURCES)
endif

if GCONFIG_WINDOWS
 libmain_a_SOURCES = $(WINDOWS_LIBMAIN_SOURCES) $(LIBMAIN_SOURCES)
else
 libmain_a_SOURCES = $(UNIX_LIBMAIN_SOURCES) $(LIBMAIN_SOURCES)
endif

if GCONFIG_WINDOWS
 RESOURCES = emailrelay.o
 $(RESOURCES): emailrelay.rc MSG00001.bin messages.rc
	$(GCONFIG_WINDRES) --preprocessor "$(CC) -E -xc-header -DRC_INVOKED" -D_WIN32=1 --include-dir . --include-dir icon --include-dir $(top_srcdir)/src/main/icon -i $(top_srcdir)/src/main/emailrelay.rc -o $(RESOURCES)
 emailrelay_SOURCES = $(WINDOWS_MAIN_SOURCES) $(OTHER_SOURCES)
 emailrelay_LDFLAGS = -mwindows -static
 emailrelay_LDADD = \
	libmain.a \
	$(top_builddir)/src/gpop/libgpop.a \
	$(top_builddir)/src/gsmtp/libgsmtp.a \
	$(top_builddir)/src/gauth/libgauth.a \
	$(top_builddir)/src/gnet/libgnet.a \
	$(top_builddir)/src/gssl/libgssl.a \
	$(top_builddir)/src/win32/libextra.a \
	$(top_builddir)/src/glib/libglib.a \
	$(RESOURCES) \
	$(GCONFIG_TLS_LIBS) \
	-lws2_32 -liphlpapi -lcomctl32
else
 emailrelay_SOURCES = $(TEXTMODE_MAIN_SOURCES) $(OTHER_SOURCES)
 emailrelay_LDFLAGS =
 emailrelay_LDADD = \
	libmain.a \
	$(top_builddir)/src/gpop/libgpop.a \
	$(top_builddir)/src/gsmtp/libgsmtp.a \
	$(top_builddir)/src/gauth/libgauth.a \
	$(top_builddir)/src/gnet/libgnet.a \
	$(top_builddir)/src/gssl/libgssl.a \
	$(top_builddir)/src/glib/libglib.a \
	$(GCONFIG_TLS_LIBS)
endif

if GCONFIG_WINDOWS
 emailrelay_textmode_SOURCES = $(TEXTMODE_MAIN_SOURCES) $(OTHER_SOURCES)
 emailrelay_textmode_LDFLAGS = -static
 emailrelay_textmode_LDADD = \
	libmain.a \
	$(top_builddir)/src/gpop/libgpop.a \
	$(top_builddir)/src/gsmtp/libgsmtp.a \
	$(top_builddir)/src/gauth/libgauth.a \
	$(top_builddir)/src/gnet/libgnet.a \
	$(top_builddir)/src/gssl/libgssl.a \
	$(top_builddir)/src/win32/libextra.a \
	$(top_builddir)/src/glib/libglib.a \
	$(GCONFIG_TLS_LIBS) \
	-lws2_32 -liphlpapi -lcomctl32
endif

emailrelay_passwd_SOURCES = passwd.cpp legal.cpp legal.h
if GCONFIG_WINDOWS
 emailrelay_passwd_LDFLAGS = -static
 emailrelay_passwd_LDADD = \
	$(top_builddir)/src/gssl/libgssl.a \
	$(top_builddir)/src/glib/libglib.a \
	$(GCONFIG_TLS_LIBS) \
	-lws2_32 -liphlpapi -lcomctl32
else
 emailrelay_passwd_LDFLAGS =
 emailrelay_passwd_LDADD =  \
	$(top_builddir)/src/gssl/libgssl.a \
	$(top_builddir)/src/glib/libglib.a \
	$(GCONFIG_TLS_LIBS)
endif

emailrelay_submit_SOURCES = submit.cpp legal.cpp legal.h
if GCONFIG_WINDOWS
 emailrelay_submit_LDFLAGS = -static
 emailrelay_submit_LDADD = \
	$(top_builddir)/src/gsmtp/libgsmtp.a \
	$(top_builddir)/src/gauth/libgauth.a \
	$(top_builddir)/src/gnet/libgnet.a \
	$(top_builddir)/src/win32/libextra.a \
	$(top_builddir)/src/glib/libglib.a \
	-lws2_32 -liphlpapi -lcomctl32
else
 emailrelay_submit_LDFLAGS =
 emailrelay_submit_LDADD = \
	$(top_builddir)/src/gsmtp/libgsmtp.a \
	$(top_builddir)/src/gauth/libgauth.a \
	$(top_builddir)/src/gnet/libgnet.a \
	$(top_builddir)/src/glib/libglib.a
endif

emailrelay_filter_copy_SOURCES = filter.cpp legal.cpp legal.h
if GCONFIG_WINDOWS
 emailrelay_filter_copy_LDFLAGS = -static
 emailrelay_filter_copy_LDADD = \
	$(top_builddir)/src/glib/libglib.a \
	-lws2_32 -liphlpapi -lcomctl32
else
 emailrelay_filter_copy_LDFLAGS =
 emailrelay_filter_copy_LDADD = \
	$(top_builddir)/src/glib/libglib.a
endif

emailrelay_start_SOURCES = start.cpp
if GCONFIG_WINDOWS
 emailrelay_start_LDFLAGS = -static
 emailrelay_start_LDADD = \
	-lws2_32 -liphlpapi -lcomctl32
else
 emailrelay_start_LDFLAGS =
 emailrelay_start_LDADD =
endif

if GCONFIG_WINDOWS
 emailrelay_service_SOURCES = \
	$(WINDOWS_SERVICEWRAPPER_SOURCES)
 emailrelay_service_LDFLAGS = -static
 emailrelay_service_LDADD = \
	libmain.a \
	$(top_builddir)/src/glib/libglib.a \
	-lws2_32 -liphlpapi -lcomctl32
endif

install-data-local: install-sbinPROGRAMS install-e_spoolDATA install-e_libexecPROGRAMS
	-mkdir -p "$(DESTDIR)$(e_spooldir)"
	-chgrp daemon "$(DESTDIR)$(e_spooldir)"
	-chmod 775 "$(DESTDIR)$(e_spooldir)"
	-chmod g+s "$(DESTDIR)$(e_spooldir)"
	-chgrp daemon "$(DESTDIR)$(sbindir)/emailrelay-submit"
	-chmod g+s "$(DESTDIR)$(sbindir)/emailrelay-submit"
	-chgrp daemon "$(DESTDIR)$(e_libexecdir)/emailrelay-filter-copy"
	-chmod g+s "$(DESTDIR)$(e_libexecdir)/emailrelay-filter-copy"

install-exec-local:
	-mkdir -p "$(DESTDIR)$(sbindir)"

uninstall-local:
	-rmdir "$(DESTDIR)$(e_spooldir)" 2>/dev/null
	-rmdir "$(DESTDIR)$(e_libexecdir)" 2>/dev/null

MSG00001.bin messages.rc: fakemc.exe messages.mc
	$(GCONFIG_WINDMC) $(top_srcdir)/src/main/messages.mc

fakemc.exe: $(top_srcdir)/src/main/fakemc.c
	$(CC) -o fakemc.exe $(top_srcdir)/src/main/fakemc.c

.PHONY: strip
strip:
	$(STRIP) $(sbin_PROGRAMS) $(e_libexec_PROGRAMS)

