# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building atvclient for AppleTV"
make clean
out=$(pwd)/files
pull_source "https://github.com/samnazarko/atvclient" "$(pwd)/src"
cd src
./configure --prefix=/usr
$BUILD
make install DESTDIR=${out}
if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
strip_files "${out}"
cd ../
dpkg -b files/ appletv-remote-osmc.deb
