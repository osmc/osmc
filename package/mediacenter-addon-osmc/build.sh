#!/bin/bash
# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

. ../common.sh

echo -e "Building package mediacenter-addon-osmc"
make clean
mkdir -p files/usr/share/kodi/addons
cp -ar src/* files/usr/share/kodi/addons

if [ "$1" == "py3" ]
then

  install_package "xmlstarlet"
  verify_action

  for d in ./files/usr/share/kodi/addons/*/ ; do
    xmlstarlet ed -L -u '/addon/requires/import[@addon="xbmc.python"]/@version' -v "3.0.0" "${d}addon.xml"
  done

fi

dpkg_build files/ mediacenter-addon-osmc.deb
