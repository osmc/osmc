# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

echo -e "Building git for QEMU ARM"
out=$(pwd)/files
if [ -d files/usr ]; then rm -rf files/usr; fi
cd src
make clean
./configure --prefix=/usr --disable-pthreads
make
make install DESTDIR=${out}
if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
cd ../
dpkg -b files/ git-osmc.deb
