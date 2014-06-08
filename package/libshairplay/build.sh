# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

echo -e "Building libshairplay"
out=$(pwd)/files
if [ -f files/usr ]; then rm -rf files/usr; fi
if [ -f files-dev/usr ]; then rm -rf files-dev/usr; fi
cd src
./autogen.sh
./configure --prefix=/usr
make
make install DESTDIR=${out}
if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
cd ../
mkdir -p files-dev/usr
mv files/usr/include  files-dev/usr/
dpkg -b files/ libshairplay.deb
dpkg -b files-dev libshairplay-dev.deb
