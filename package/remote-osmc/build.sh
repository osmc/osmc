# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh
build_in_env "${1}" $(pwd) "remote-osmc"
if [ $? == 0 ]
then
	echo -e "Building remote package"
	make clean
	sed '/Package/d' -i files/DEBIAN/control
	sed '/Depends/d' -i files/DEBIAN/control
	echo "Package: ${1}-remote-osmc" >> files/DEBIAN/control
	echo "Depends: ${1}-lirc-osmc, ${1}-eventlircd-osmc" >> files/DEBIAN/control
	fix_arch_ctl "files/DEBIAN/control"
	dpkg -b files/ remote-osmc.deb
fi
teardown_env "${1}"
