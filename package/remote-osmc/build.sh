# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh
build_in_env "${1}" $(pwd) "remote-osmc"
build_return=$?
if [ $build_return == 99 ]
then
	echo -e "Building remote package"
	make clean
	sed '/Package/d' -i files/DEBIAN/control
	sed '/Depends/d' -i files/DEBIAN/control
	echo "Package: ${1}-remote-osmc" >> files/DEBIAN/control
	echo "Depends: ${1}-lirc-osmc, ${1}-eventlircd-osmc" >> files/DEBIAN/control
	fix_arch_ctl "files/DEBIAN/control"
	dpkg_build files/ ${1}-remote-osmc.deb
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
