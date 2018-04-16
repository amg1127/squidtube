TEMPLATE = subdirs
SUBDIRS += common helpers src

###########################
# Make test support

include (project-defs.pri)
test.commands = php -d allow_url_fopen=1 tests/test.php $${project_name}
QMAKE_EXTRA_TARGETS += test
