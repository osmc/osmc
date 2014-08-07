# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building FTP server"
dpkg -b files/ ftp-osmc.deb
