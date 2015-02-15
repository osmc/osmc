# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

WD="/tmp/fw-dump"

function install_fw_from_deb()
{
	DEB_BASE="http://ftp.debian.org/debian/pool/non-free/f/firmware-nonfree"
	if [ -d $WD ]; then rm -rf $WD > /dev/null 2>&1; fi
	mkdir -p $WD
	pushd $WD
	wget ${DEB_BASE}/${1}
	if [ $? != 0 ]; then echo "Firmware download failed" && exit 1; fi
	dpkg -x ${1} .
	popd
	cp -ar $WD/lib/firmware/* files/lib/firmware
}

echo -e "Building package WiFi firmware"

make clean

RALINK_DEB="firmware-ralink_0.43_all.deb"
REALTEK_DEB="firmware-realtek_0.43_all.deb"
ATHEROS_DEB="firmware-atheros_0.43_all.deb"
BRCM_DEB="firmware-brcm80211_0.43_all.deb"

mkdir -p files/lib/firmware
install_fw_from_deb ${RALINK_DEB}
install_fw_from_deb ${REALTEK_DEB}
install_fw_from_deb ${ATHEROS_DEB}
install_fw_from_deb ${BRCM_DEB}

dpkg -b files/ wireless-firmware-osmc.deb
