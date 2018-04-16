TEMPLATE = subdirs
SUBDIRS += common helpers src tests

###########################
# Make test support

include (project-defs.pri)
test.commands = $(MAKE) -C tests test
test.depends = build
build.commands = $(MAKE)
QMAKE_EXTRA_TARGETS += test build
