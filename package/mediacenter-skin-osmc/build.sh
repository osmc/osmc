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

REV="8be827eafef0e100e05dc00b86929cf911edf22b"
SCOPE_REV="a2a91416c852a6eca5a4940a7c478b92989b091b"
FOURTOTHREE_REV="35301b1403a1ea545f64084d50caadf5ef4254d3"
TWENTYONETONINE_REV="168364ed6cfbc355b7e049ec72f0dfe8072da551"
echo -e "Building package mediacenter-skin-osmc"
echo -e "Downloading skin"
pull_source "https://github.com/osmc/skin.osmc/archive/${REV}.tar.gz" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error downloading classic skin" && exit 1; fi
process_skin "$REV" "skin.osmc"
echo -e "Downloading scope skin"
pull_source "https://github.com/osmc/skin.osmc/archive/${SCOPE_REV}.tar.gz" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error downloading scope skin" && exit 1; fi
process_skin "$SCOPE_REV" "skin.osmc.scope"
echo -e "Downloading 4:3 skin"
pull_source "https://github.com/osmc/skin.osmc/archive/${FOURTOTHREE_REV}.tar.gz" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error downloading 4:3 skin" && exit 1; fi
process_skin "$FOURTOTHREE_REV" "skin.osmc.4to3"
echo -e "Downloading 21:9 skin"
pull_source "https://github.com/osmc/skin.osmc/archive/${TWENTYONETONINE_REV}.tar.gz" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error downloading 21:9 skin" && exit 1; fi
process_skin "$TWENTYONETONINE_REV" "skin.osmc.21to9"
dpkg_build files/ mediacenter-skin-osmc.deb
