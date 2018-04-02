QT -= gui
QT += core network qml sql

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

# Turn all warnings on
CONFIG += warn_on
QMAKE_CXXFLAGS += -Wall -Wextra -Wconversion
DEFINES += QT_DEPRECATED_WARNINGS

###########################
# Make project variables available inside the C++ code
DEFINES += APP_project_name=\"\\\"$${project_name}\\\"\" \
    APP_project_version=\"\\\"$${project_version}\\\"\" \
    APP_owner_name=\"\\\"$${owner_name}\\\"\" \
    APP_install_bin_dir=\"\\\"$${install_bin_dir}\\\"\" \
    APP_install_etc_dir=\"\\\"$${install_etc_dir}\\\"\" \
    APP_install_share_dir=\"\\\"$${install_share_dir}\\\"\"

###########################
# Make install support

# Install binaries
target.path = $${install_bin_dir}

# Install configuration files
configfiles.path = $${install_etc_dir}
configfiles.files = $$sprintf("../%1.conf", $${project_name})
INSTALLS += configfiles

DISTFILES += \
    xmlhttprequest.js \
    helperjail.js

RESOURCES += \
    javascriptsources.qrc
