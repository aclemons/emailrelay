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

SOURCES_PAM_AUTH_ = gsaslclient_native.cpp gsaslserverbasic_native.cpp gsaslserverfactory_pam.cpp gsecrets_pam.cpp gsaslserverpam.cpp gsecretsfile.cpp
SOURCES_PAM_NOAUTH_ = gsaslclient_none.cpp gsaslserverbasic_none.cpp gsaslserverfactory_native.cpp gsecrets_none.cpp
SOURCES_NOPAM_NOAUTH_ = gsaslclient_none.cpp gsaslserverbasic_none.cpp gsaslserverfactory_native.cpp gsecrets_none.cpp
SOURCES_NOPAM_AUTH_ = gsaslclient_native.cpp gsaslserverbasic_native.cpp gsaslserverfactory_native.cpp gsecrets_full.cpp gsecretsfile.cpp
EXTRA_DIST_PAM_AUTH_ = gsaslclient_none.cpp gsaslserverbasic_none.cpp gsaslserverfactory_native.cpp gsecrets_full.cpp gsecrets_none.cpp
EXTRA_DIST_PAM_NOAUTH_ = gsaslclient_native.cpp gsaslserverbasic_native.cpp gsaslserverfactory_pam.cpp gsecrets_full.cpp gsecrets_pam.cpp gsaslserverpam.cpp gsecretsfile.cpp
EXTRA_DIST_NOPAM_NOAUTH_ = gsaslclient_native.cpp gsaslserverbasic_native.cpp gsaslserverfactory_pam.cpp gsecrets_full.cpp gsecrets_pam.cpp gsaslserverpam.cpp gsecretsfile.cpp
EXTRA_DIST_NOPAM_AUTH_ = gsaslclient_none.cpp gsaslserverbasic_none.cpp gsaslserverfactory_pam.cpp gsecrets_none.cpp gsecrets_pam.cpp gsaslserverpam.cpp

if PAM
 SOURCES_AUTH_ = $(SOURCES_PAM_AUTH_)
 EXTRA_DIST_AUTH_ = $(EXTRA_DIST_PAM_AUTH_)
 SOURCES_NOAUTH_ = $(SOURCES_PAM_NOAUTH_)
 EXTRA_DIST_NOAUTH_ = $(EXTRA_DIST_PAM_NOAUTH_)
 PAM_DATA_ = emailrelay
else
 SOURCES_AUTH_ = $(SOURCES_NOPAM_AUTH_)
 EXTRA_DIST_AUTH_ = $(EXTRA_DIST_NOPAM_AUTH_)
 SOURCES_NOAUTH_ = $(SOURCES_NOPAM_NOAUTH_)
 EXTRA_DIST_NOAUTH_ = $(EXTRA_DIST_NOPAM_NOAUTH_)
 PAM_DATA_ =
endif

if AUTH
 EXTRA_DIST_ = $(EXTRA_DIST_AUTH_)
 SOURCES_ = $(SOURCES_AUTH_)
else
 EXTRA_DIST_ = $(EXTRA_DIST_NOAUTH_)
 SOURCES_ = $(EXTRA_DIST_NOAUTH_)
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

