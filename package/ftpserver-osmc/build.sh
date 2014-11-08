# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building package ftp-app-osmc"
dpkg -b files/ ftp-app-osmc.deb
