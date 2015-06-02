# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building package DVB firmware"

FW_SCRIPT="https://www.kernel.org/doc/Documentation/dvb/get_dvb_firmware"

make clean
wget $FW_SCRIPT -O get_dvb_firmware
chmod +x get_dvb_firmware
install_patch "patches/" "all"

./get_dvb_firmware
mkdir -p files/lib/firmware
mv *.fw files/lib/firmware

dpkg_build files/ dvb-firmware-osmc.deb
