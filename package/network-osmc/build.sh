# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh
build_in_env "${1}" $(pwd) "network-osmc"
if [ $? == 0 ]
then
	echo -e "Building networking package"
	make clean
	sed '/Package/d' -i files/DEBIAN/control
	sed '/Depends/d' -i files/DEBIAN/control
	echo "Package: ${1}-network-osmc" >> files/DEBIAN/control
	echo "Depends: ${1}-connman-osmc, bluez, wireless-firmware-osmc, net-tools, ntp, wpasupplicant, libnss-mdns, nfs-common, cifs-utils, ca-certificates, curl, python-dbus, python-gobject" >> files/DEBIAN/control
	fix_arch_ctl "files/DEBIAN/control"
	dpkg -b files/ network-osmc.deb
fi
teardown_env "${1}"
