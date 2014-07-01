# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

echo -e "Building armmem for Raspberry Pi"
out=$(pwd)/files
cd src
make
strip_libs
cp -ar libarmmem.so $out
cp -ar libarmmem.a $out
if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
cd ../
dpkg -b files/ rpi-armmem.deb
