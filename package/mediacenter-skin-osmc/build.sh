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

REV="25b09ec496a9320b9634ea487a695b9b2223f433"
SCOPE_REV="7834f229953f9753ea2388546c57b18270cc5cfc"
FOURTOTHREE_REV="181525c361a171194a063bf730e6fec177aa50a4"
TWENTYONETONINE_REV="ef0174380f5776237dd76540cedae88e32301b34"
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
