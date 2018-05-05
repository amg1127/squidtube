TEMPLATE = aux

###########################
# Make test support

include(../project-defs.pri)
test.commands = php -d allow_url_fopen=1 test.php $${project_name}
test.depends = build
build.commands = $(MAKE) -C ..
QMAKE_EXTRA_TARGETS += test build

DISTFILES += \
    test.php \
    test_config.conf \
    helpers/test_xhr.js \
    suite/*.php

QMAKE_CLEAN += test.stdin test.stdout test.log test.sqlite  test.sqlite-shm  test.sqlite-wal
