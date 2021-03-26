#!/bin/bash
# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

. ../common.sh

echo -e "Building package mediacenter-addon-osmc"
make clean
mkdir -p files/usr/share/kodi/addons
cp -ar src/* files/usr/share/kodi/addons

dpkg_build files/ mediacenter-addon-osmc.deb
