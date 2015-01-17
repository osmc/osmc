# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building remote package"
make clean
sed '/Package/d' -i files/DEBIAN/control
sed '/Depends/d' -i files/DEBIAN/control
test "$1" == rbp && echo "Package: rbp-remote-osmc" >> files/DEBIAN/control
test "$1" == rbp && echo "Depends: lirc, rbp-eventlircd-osmc" >> files/DEBIAN/control
fix_arch_ctl "files/DEBIAN/control"
dpkg -b files/ remote-osmc.deb
