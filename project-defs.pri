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
