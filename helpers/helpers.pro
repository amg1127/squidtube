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

include(../project-defs.pri)

TEMPLATE = aux

###########################
# Make install support

helperfiles.path = $$sprintf("%1/helpers", $${install_share_dir})
helperfiles.files = _TEMPLATE_.js youtube.js flickr.js
INSTALLS += helperfiles
