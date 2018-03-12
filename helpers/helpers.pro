include(../project-defs.pri)

TEMPLATE = aux

###########################
# Make install support

helperfiles.path = $$sprintf("%1/helpers", $${install_share_dir})
helperfiles.files = *.js
INSTALLS += helperfiles
