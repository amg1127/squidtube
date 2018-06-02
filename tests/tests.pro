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
