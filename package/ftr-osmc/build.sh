# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building ftr"
make clean
pushd files
test "$1" == rbp1 && install_patch "../patches" "rbp1"
test "$1" == rbp2 && install_patch "../patches" "rbp1"
test "$1" == atv && install_patch "../patches" "atv"
popd
dpkg -b files/ ftr-osmc.deb
