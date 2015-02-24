# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../../scripts/common.sh

echo -e "Downloading latest filesystem for Raspberry Pi"
date=$(date +%Y%m%d)
count=150
while [ $count -gt 0 ]; do wget --spider -q ${DOWNLOAD_URL}/filesystems/osmc-rbp-filesystem-${date}.tar.xz
	if [ "$?" -eq 0 ]; then
		wget ${DOWNLOAD_URL}/filesystems/osmc-rbp-filesystem-${date}.tar.xz -O filesystem.tar.xz
		break
	fi
	date=$(date +%Y%m%d --date "yesterday $date")
	let count=count-1
	done
if [ ! -f filesystem.tar.xz ]; then echo -e "No filesystem available for target" && exit 1; fi
echo -e "Extracting filesystem"
mkdir output
tar -xJf filesystem.tar.xz -C output/
rm -rf filesystem.tar.xz
pushd output
pushd boot
# Add Pi1 config.txt
echo "arm_freq=850
core_freq=375
gpu_mem_256=112
gpu_mem_512=144
hdmi_ignore_cec_init=1
disable_overscan=1
start_x=1" > config1.txt
# Add Pi2 config.txt
echo "gpu_mem_1024=256
hdmi_ignore_cec_init=1
disable_overscan=1
start_x=1" > config2.txt
tar -cf - * | xz -9 -c - > boot.tar.xz
mv boot.tar.xz ../../
popd
# NOOBS modifications, i.e. future 'health' script would be in .
tar -cf - * | xz -9 -c - > root.tar.xz
mv root.tar.xz ../
popd
echo -e "Build completed"
