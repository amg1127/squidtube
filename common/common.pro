include(../project-defs.pri)

TEMPLATE = aux

###########################
# Make install support

commonfiles.path = $$sprintf("%1/common", $${install_share_dir})
commonfiles.files = URL.js
INSTALLS += commonfiles
