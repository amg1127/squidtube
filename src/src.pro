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

QT -= gui
QT += core network qml sql
win32:CONFIG += console

include(../project-defs.pri)

TARGET = $${project_name}
DESTDIR = ..
builddir = ../.build/src
OBJECTS_DIR = $${builddir}
MOC_DIR = $${builddir}
RCC_DIR = $${builddir}
UI_DIR = $${builddir}

TEMPLATE = app
VERSION = $${project_version}

SOURCES += main.cpp \
    appconfig.cpp \
    appruntime.cpp \
    databasebridge.cpp \
    stdinreader.cpp \
    jobdispatcher.cpp \
    jobworker.cpp \
    javascriptbridge.cpp \
    objectcache.cpp

HEADERS  += appconfig.h \
    appruntime.h \
    databasebridge.h \
    stdinreader.h \
    jobdispatcher.h \
    jobworker.h \
    javascriptbridge.h \
    objectcache.h

# Turn warnings into errors
unix {
    QMAKE_CXXFLAGS_RELEASE += -Werror
}

# What is the compiler? GCC? Clang? Neither?
compilerIsGcc = $$find(QMAKE_CXX,(|.*-)g\+\+)
compilerIsClang = $$find(QMAKE_CXX,(|.*-)clang\+\+)

# Turn all warnings on
CONFIG += warn_on
QMAKE_CXXFLAGS += -Wall -Wextra -Wconversion
!isEmpty(compilerIsClang):!isEmpty(CLANG_WARN_EVERYTHING) {
    QMAKE_CXXFLAGS += -Weverything \
        -Wno-c++98-compat \
        -Wno-exit-time-destructors \
        -Wno-global-constructors \
        -Wno-covered-switch-default \
        -Wno-padded -Wno-switch-enum \
        -Wno-redundant-parens
}
DEFINES += QT_DEPRECATED_WARNINGS

# Abort compilation if there are more than 3 errors
#count(compilerIsGcc,1):QMAKE_CXXFLAGS += -fmax-errors=3
#count(compilerIsClang,1):QMAKE_CXXFLAGS += -ferror-limit=3

# I want to handle character conversion explicitly
DEFINES += QT_NO_CAST_FROM_ASCII QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_BYTEARRAY

# Defines the C++ standard
CONFIG += c++11

###########################
# Make project variables available inside the C++ code
DEFINES += APP_project_name=\"\\\"$${project_name}\\\"\" \
    APP_project_version=\"\\\"$${project_version}\\\"\" \
    APP_owner_name=\"\\\"$${owner_name}\\\"\" \
    APP_project_url=\"\\\"$${project_url}\\\"\" \
    APP_install_bin_dir=\"\\\"$${install_bin_dir}\\\"\" \
    APP_install_etc_dir=\"\\\"$${install_etc_dir}\\\"\" \
    APP_install_share_dir=\"\\\"$${install_share_dir}\\\"\"

###########################
# Make install support

# Install binaries
target.path = $${install_bin_dir}
INSTALLS += target

# Install configuration files
configfiles.path = $${install_etc_dir}
configfiles.files = $$sprintf("../%1.conf", $${project_name})
INSTALLS += configfiles

DISTFILES += \
    xmlhttprequest.js \
    helperjailer.js

RESOURCES += \
    javascriptsources.qrc
