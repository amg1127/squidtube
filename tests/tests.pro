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

TEMPLATE = aux

###########################
# Make test support

include(../project-defs.pri)
win32 {
    phplibprefix = php_
    phplibsuffix = .dll
} else {
    phplibprefix =
    phplibsuffix = .so
}
test.commands = php -d extension=$${phplibprefix}sockets$${phplibsuffix} -d extension=sockets -d allow_url_fopen=1 test.php $${project_name}
test.depends = build
build.commands = $(MAKE) -C ..
QMAKE_EXTRA_TARGETS += test build

DISTFILES += \
    test.php \
    test_config.conf \
    suite/*.php \
    helpers/testhelper.js

QMAKE_CLEAN += test.stdin test.stdout test.log test.sqlite  test.sqlite-shm  test.sqlite-wal
