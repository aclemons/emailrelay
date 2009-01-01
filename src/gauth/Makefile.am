#
## Copyright (C) 2001-2009 Graeme Walker <graeme_walker@users.sourceforge.net>
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

if PAM
 EXTRA_DIST__ = gsaslserverfactory_native.cpp gsecrets_none.cpp
 SOURCES__ = gsaslserverfactory_pam.cpp gsaslserverpam.cpp gsecrets_pam.cpp
else
 EXTRA_DIST__ = gsaslserverfactory_pam.cpp gsaslserverpam.cpp gsecrets_pam.cpp
 SOURCES__ = gsaslserverfactory_native.cpp gsecrets_none.cpp
endif

if AUTH
 EXTRA_DIST_ = $(EXTRA_DIST__) gsaslclient_none.cpp gsaslserverbasic_none.cpp gsecrets_full.cpp
 SOURCES_ = $(SOURCES__) gsaslclient_native.cpp gsaslserverbasic_native.cpp gsecretsfile.cpp
else
 EXTRA_DIST_ = gsecretsfile.cpp gsecrets_pam.cpp gsaslserverfactory_pam.cpp gsaslserverpam.cpp gsecrets_full.cpp gsaslclient_native.cpp gsaslserverbasic_native.cpp 
 SOURCES_ = gsaslserverfactory_native.cpp gsecrets_none.cpp gsaslclient_none.cpp gsaslserverbasic_none.cpp
endif

EXTRA_DIST=mingw.mak $(EXTRA_DIST_)

INCLUDES = -I$(top_srcdir)/lib/$(COMPILER_VERSION) -I$(top_srcdir)/src/glib -I$(top_srcdir)/src/gssl -I$(top_srcdir)/src/gnet -DG_SPOOLDIR=\"$(e_spooldir)\"

noinst_LIBRARIES = libgauth.a

libgauth_a_SOURCES = \
	$(SOURCES_) \
	gauth.h \
	gsaslclient.h \
	gsaslserverbasic.h \
	gsaslserverfactory.h \
	gsaslserver.cpp \
	gsaslserver.h \
	gsaslserverpam.h \
	gsecretsfile.h \
	gsecrets.h \
	gvalid.cpp \
	gvalid.h

