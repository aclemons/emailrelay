#
## Copyright (C) 2001-2020 Graeme Walker <graeme_walker@users.sourceforge.net>
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

SUBDIRS = glib gssl gnet gauth gsmtp gpop win32 main gui
EXTRA_DIST = .clang-tidy
CLEANFILES = compile_commands.json

LINT = clang-tidy

.PHONY: cdb
cdb: compile_commands.json

compile_commands.json:
	@chmod +x ../bin/make2cdb || true
	../bin/make2cdb > compile_commands.json

.PHONY: lint
lint: compile_commands.json
	@chmod +x ../bin/make2cdb || true
	@touch ../gconfig_defs.h
	../bin/make2cdb --list | xargs $(LINT) -p . || true

