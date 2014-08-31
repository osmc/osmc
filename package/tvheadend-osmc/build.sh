# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building TVHeadend"
out=$(pwd)/files
sed '/Package/d' -i files/DEBIAN/control
test $1 == gen && echo "Package: tvheadend-osmc-app" >> files/DEBIAN/control
test $1 == rbp && echo "Package: rbp-tvheadend-osmc-app" >> files/DEBIAN/control
pull_source "https://github.com/tvheadend/tvheadend" "$(pwd)/src"
cd src
git checkout v3.9
./configure --prefix=/usr
$BUILD
make install DESTDIR=${out}
if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
strip_files "${out}"
cd ../
fix_arch_ctl "files/DEBIAN/control"
dpkg -b files/ tvheadend-osmc.deb
