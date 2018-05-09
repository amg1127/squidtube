TEMPLATE = aux

###########################
# Make test support

include(../project-defs.pri)
win32 {
    phplibsuffix = .dll
} else {
    phplibsuffix = .so
}
test.commands = php -d extension=sockets$${phplibsuffix} -d allow_url_fopen=1 test.php $${project_name}
test.depends = build
build.commands = $(MAKE) -C ..
QMAKE_EXTRA_TARGETS += test build

DISTFILES += \
    test.php \
    test_config.conf \
    suite/*.php \
    helpers/testhelper.js

QMAKE_CLEAN += test.stdin test.stdout test.log test.sqlite  test.sqlite-shm  test.sqlite-wal
