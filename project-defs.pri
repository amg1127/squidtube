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

# Project information
project_name = squidtube
project_version = 1.0.0
owner_name = amg1127
project_url = $$sprintf("https://github.com/%2/%1/", $${project_name}, $${owner_name})

# Default path for binaries
isEmpty (install_bin_dir) {
    unix:install_bin_dir = /usr/local/bin
    win32:install_bin_dir = $$sprintf("C:/Program Files/%2/%1", $${project_name}, $${owner_name})
}

# Default path for configuration files
isEmpty (install_etc_dir) {
    unix:install_etc_dir = $$sprintf("/usr/local/etc/%1", $${project_name})
    win32:install_etc_dir = $$sprintf("C:/Program Files/%2/%1", $${project_name}, $${owner_name})
}

# Default path for architecture independent data files
isEmpty (install_share_dir) {
    unix:install_share_dir = $$sprintf("/usr/local/share/%1", $${project_name})
    win32:install_share_dir = $$sprintf("C:/Program Data/%2/%1", $${project_name}, $${owner_name})
}
