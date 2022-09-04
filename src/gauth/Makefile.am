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

AM_CPPFLAGS = \
	-I$(top_srcdir)/src/glib \
	-I$(top_srcdir)/src/gssl \
	-I$(top_srcdir)/src/gnet \
	-D "G_SPOOLDIR=$(e_spooldir)"

noinst_LIBRARIES = libgauth.a

if GCONFIG_PAM
 PAM_SOURCES = \
	gsaslserverfactory_pam.cpp \
	gsaslserverpam.cpp \
	gsaslserverpam.h
 PAM_EXTRA_DIST = \
	gsaslserverfactory_basic.cpp
else
 PAM_SOURCES = \
	gsaslserverfactory_basic.cpp
 PAM_EXTRA_DIST = \
	gsaslserverfactory_pam.cpp \
	gsaslserverpam.cpp \
	gsaslserverpam.h
endif

EXTRA_DIST = \
	$(PAM_EXTRA_DIST)

libgauth_a_SOURCES = \
	$(PAM_SOURCES) \
	gcram.h \
	gcram.cpp \
	gsaslclient.h \
	gsaslclient.cpp \
	gsaslclientsecrets.h \
	gsaslclientsecrets.cpp \
	gsaslserver.h \
	gsaslserver.cpp \
	gsaslserversecrets.h \
	gsaslserversecrets.cpp \
	gsaslserverbasic.h \
	gsaslserverbasic.cpp \
	gsaslserverfactory.h \
	gsecret.h \
	gsecret.cpp \
	gsecrets.h \
	gsecrets.cpp \
	gsecretsfile.cpp \
	gsecretsfile.h \
	gvalid.cpp \
	gvalid.h

