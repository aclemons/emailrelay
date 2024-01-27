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

SUBDIRS = glib gssl gnet gauth gstore gsmtp gpop win32 gfilters gverifiers main gui
EXTRA_DIST = .clang-tidy .clang-format
CLEANFILES = compile_commands.json

TIDY ?= clang-tidy

.PHONY: cdb
cdb: compile_commands.json

compile_commands.json: Makefile
	@chmod +x $(top_srcdir)/libexec/make2cdb || true
	$(top_srcdir)/libexec/make2cdb > $@.tmp && mv $@.tmp $@

.PHONY: tidy
tidy: compile_commands.json
	@chmod +x $(top_srcdir)/libexec/make2cdb || true
	@touch $(top_srcdir)/gconfig_defs.h
	$(top_srcdir)/libexec/make2cdb --list | xargs -- "$(TIDY)" -p . || true

.PHONY: format
format:
	@chmod +x $(top_srcdir)/libexec/make-format || true
	$(top_srcdir)/libexec/make-format -f "$(FORMAT)" -d $(top_srcdir)

