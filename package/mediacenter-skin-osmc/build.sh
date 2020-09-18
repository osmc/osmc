# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

make clean

process_skin() {
echo -e "Moving ${2} files in to place"
mkdir -p files/usr/share/kodi/addons
cp -ar src/skin.osmc-${1} files/usr/share/kodi/addons/${2} # Always called skin.osmc in src because of repo name
if [ -f files/usr/share/kodi/addons/${2}/media/Textures.xbt ]
then
    echo "TexturePacked file detected, deleting unneeded artefacts"
    pushd files/usr/share/kodi/addons/${2}/media
    find . ! -name 'Textures.xbt' -delete
    popd
fi
}

REV="7905fc9a449a4d3c03d4f7a813a55deca91b570e"
SCOPE_REV="f76e68061660ec799aad2e3185db77a8323be3a7"
echo -e "Building package mediacenter-skin-osmc"
echo -e "Downloading skin"
pull_source "https://github.com/osmc/skin.osmc/archive/${REV}.tar.gz" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error downloading classic skin" && exit 1; fi
process_skin "$REV" "skin.osmc"
echo -e "Downloading scope skin"
pull_source "https://github.com/osmc/skin.osmc/archive/${SCOPE_REV}.tar.gz" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error downloading classic skin" && exit 1; fi
process_skin "$SCOPE_REV" "skin.osmc.scope"
dpkg_build files/ mediacenter-skin-osmc.deb
