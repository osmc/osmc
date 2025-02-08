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

REV="5327f8cfb775de1e6f28981576b0c525714ab446"
SCOPE_REV="68f83d22e48ff4ca1795b418f78eaa597f44872f"
FOURTOTHREE_REV="80cef727ce5001509ced24e6a226dc43b6f6f1db"
TWENTYONETONINE_REV="f903f9b42fe421da95d47253285a4e4297417a80"
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
