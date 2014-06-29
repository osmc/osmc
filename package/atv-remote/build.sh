# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

echo -e "Building atvclient for AppleTV"
out=$(pwd)/files
cd src
./configure --prefix=/usr
make
make install DESTDIR=${out}
strip_files "${out}"
if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
cd ../
dpkg -b files/ osmc-appletv-remote.deb
