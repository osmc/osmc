#!/bin/bash
# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

. ../common.sh

REV="755ec480938e40e2b3effd109d1bdf7fc0331184"
echo -e "Building package mediacenter-addon-globalsearch-osmc"
make clean
mkdir -p files/usr/share/kodi/addons
pull_source "https://gitlab.com/ronie/script.globalsearch/-/archive/${REV}/script.globalsearch-master.tar.gz" "files/usr/share/kodi/addons"
mv files/usr/share/kodi/addons/script.globalsearch* files/usr/share/kodi/addons/script.globalsearch

dpkg_build files/ mediacenter-addon-globalsearch-osmc.deb
