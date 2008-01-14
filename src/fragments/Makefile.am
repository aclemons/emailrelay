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
noinst_LIBRARIES = libfragments.a
INCLUDES = -I$(top_srcdir)/lib/$(COMPILER_VERSION) 
INCLUDES += -I$(top_srcdir)/src/glib  
INCLUDES += -I$(top_srcdir)/src/gssl
INCLUDES += -I$(top_srcdir)/src/gnet
libfragments_a_SOURCES = empty.cpp
libfragments_a_LIBADD = @FRAGMENTS_LIST@
libfragments_a_DEPENDENCIES = @FRAGMENTS_LIST@
distclean-local:
	$(RM) g*.cpp
