# squidtube - An external Squid ACL class helper that provides control over access to videos
# Copyright (C) 2018  Anderson M. Gomes
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

TEMPLATE = subdirs
SUBDIRS += common helpers src tests

###########################
# Make test support

include (project-defs.pri)
test.commands = $(MAKE) -C tests test
test.depends = build
build.commands = $(MAKE)
QMAKE_EXTRA_TARGETS += test build
