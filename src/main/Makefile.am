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

# force symbol stripping on 'make install' -- see also make 'install-strip'
AM_INSTALL_PROGRAM_FLAGS=-s

EXTRA_DIST=\
	common.dsp \
	doxygen.cfg \
	doxygen.h \
	emailrelay.dsp \
	emailrelay.rc \
	icon-32.ico \
	icon2.ico \
	icon3.ico \
	icon4.ico \
	icon5.ico \
	messages.mc \
	news.cpp \
	news.h \
	passwd.dsp \
	poke.dsp \
	resource.h \
	scanner.cpp \
	submit.dsp \
	winapp.cpp \
	winapp.h \
	winform.cpp \
	winform.h \
	winmain.cpp \
	winmenu.cpp \
	winmenu.h \
	mingw.c \
	mingw.mak

INCLUDES = -I$(top_srcdir)/lib/$(COMPILER_VERSION) -I$(top_srcdir)/src/glib -I$(top_srcdir)/src/gnet -I$(top_srcdir)/src/gsmtp -I$(top_srcdir)/src/gpop -DG_SPOOLDIR=\"$(e_spooldir)\"

sbin_PROGRAMS = emailrelay emailrelay-submit emailrelay-passwd
libexec_PROGRAMS = emailrelay-poke emailrelay-filter-copy
e_spool_DATA = 

emailrelay_SOURCES = \
	commandline.cpp \
	commandline.h \
	configuration.cpp \
	configuration.h \
	legal.cpp \
	legal.h \
	main.cpp \
	output.cpp \
	output.h \
	run.cpp \
	run.h

emailrelay_LDADD = \
	$(top_builddir)/src/gpop/$(POPLIB) \
	$(top_builddir)/src/gsmtp/libgsmtp.a \
	$(top_builddir)/src/glib/libglib.a \
	$(top_builddir)/src/gnet/libgnet.a \
	$(top_builddir)/src/gnet/lib$(IP).a

emailrelay_poke_SOURCES = poke.c
emailrelay_poke_LDADD =

emailrelay_passwd_SOURCES = passwd.cpp legal.cpp
emailrelay_passwd_LDADD = $(top_builddir)/src/glib/libglib.a

emailrelay_submit_SOURCES = submit.cpp legal.cpp
emailrelay_submit_LDADD = \
	$(top_builddir)/src/gsmtp/libgsmtp.a \
	$(top_builddir)/src/glib/libglib.a \
	$(top_builddir)/src/gnet/libgnet.a \
	$(top_builddir)/src/gnet/lib$(IP).a

emailrelay_filter_copy_SOURCES = filter_copy.cpp
emailrelay_filter_copy_LDADD = \
	$(top_builddir)/src/glib/libglib.a

install-data-local: install-sbinPROGRAMS install-e_spoolDATA install-libexecPROGRAMS
	-chgrp daemon $(DESTDIR)$(e_spooldir)
	-chmod 775 $(DESTDIR)$(e_spooldir)
	-chgrp daemon $(DESTDIR)$(sbindir)/emailrelay-submit
	-chmod g+s $(DESTDIR)$(sbindir)/emailrelay-submit

install-exec-local:

uninstall-local:
	-rmdir $(DESTDIR)$(e_spooldir) 2>/dev/null
	-rmdir $(DESTDIR)$(libexecdir) 2>/dev/null

