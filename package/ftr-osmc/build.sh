# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh
build_in_env "${1}" $(pwd) "ftr-osmc"
if [ $? == 0 ]
then
	echo -e "Building ftr"
	make clean
	sed '/Package/d' -i files/DEBIAN/control
	echo "Package: ${1}-ftr-osmc" >> files/DEBIAN/control
	fix_arch_ctl "files/DEBIAN/control"
    cp src/${1}-ftr files/usr/bin/ftr
	pushd files
	chmod +x usr/bin/ftr
	popd
	dpkg -b files/ ftr-osmc.deb
fi
teardown_env "${1}"
