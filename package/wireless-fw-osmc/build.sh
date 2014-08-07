# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

function install_fw()
{
	DEB_BASE="http://ftp.debian.org/debian/pool/non-free/f/firmware-nonfree/"
	if [ -d $WD ]; then rm -rf $WD > /dev/null 2>&1; fi
	WD="/tmp/fw-dump"
	mkdir -p $WD
	pushd $WD
	wget ${DEB_BASE}$1
	if [ $? != 0 ]; then echo "Firmware download failed" && exit 1; fi
	dpkg -x ${1} .
	popd
	cp -ar $WD/lib/firmware/* files/lib/firmware
}

echo -e "Building package WiFi firmware"

make clean

RALINK_DEB="firmware-ralink_0.43_all.deb"
REALTEK_DEB="firmware-realtek_0.43_all.deb"

mkdir -p files/lib/firmware
install_fw ${RALINK_DEB}
install_fw ${REALTEK_DEB}

dpkg -b files/ osmc-wifi-firmware.deb
