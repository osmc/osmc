# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

echo -e "Building package DVB firmware"

if [ -d files/lib/firmware ]; then echo "Removing old firmware" && rm -rf files/lib/firmware > /dev/null 2>&1; fi
rm *.fw > /dev/null 2>&1

FW_SCRIPT="https://www.kernel.org/doc/Documentation/dvb/get_dvb_firmware"

wget $FW_SCRIPT -O get_dvb_firmware
chmod +x get_dvb_firmware
patch -p1 < 000-pull_firmware_auto.patch

./get_dvb_firmware
mkdir -p files/lib/firmware
mv *.fw files/lib/firmware

dpkg -b files/ osmc-dvb-firmware.deb
