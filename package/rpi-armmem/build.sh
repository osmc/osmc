# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

echo -e "Building armmem for Raspberry Pi"
out=$(pwd)/files
cd src
make
if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
strip_libs
mkdir -p $out/usr/lib
cp -ar libarmmem.so $out/usr/lib
cp -ar libarmmem.a $out/usr/lib
cd ../
dpkg -b files/ rpi-armmem.deb
