# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../../scripts/common.sh

function build_fs_image()
{
	echo -e "Downloading latest filesystem for RBP Version ${1}"
	date=$(date +%Y%m%d)
	count=150
	while [ $count -gt 0 ]; do wget --spider -q ${DOWNLOAD_URL}/filesystems/osmc-${1}-filesystem-${date}.tar.xz
		   if [ "$?" -eq 0 ]; then
				wget ${DOWNLOAD_URL}/filesystems/osmc-${1}-filesystem-${date}.tar.xz -O filesystem.tar.xz
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
	if [ "$1" == "rbp1" ]
	then
		# Add Pi1 config.txt
		echo "arm_freq=850" >> config.txt
		echo "core_freq=375" >> config.txt
		echo "gpu_mem_256=112" >> config.txt
		echo "gpu_mem_512=144" >> config.txt
		echo "hdmi_ignore_cec_init=1" >> config.txt
		echo "disable_overscan=1" >> config.txt
		echo "start_x=1" >> config.txt
		echo "dtoverlay=lirc-rpi:gpio_out_pin=17,gpio_in_pin=18" >> config.txt
		echo "disable_splash=1" >> config.txt
	else
		# Add Pi2 config.txt
		echo "gpu_mem_1024=256" >> config.txt
		echo "hdmi_ignore_cec_init=1" >> config.txt
		echo "disable_overscan=1" >> config.txt
		echo "start_x=1" >> config.txt
		echo "dtoverlay=lirc-rpi:gpio_out_pin=17,gpio_in_pin=18" >> config.txt
		echo "disable_splash=1" >> config.txt
	fi
	echo -e "Creating boot tarball"
	tar -cf - * | xz -9 -c - > boot-${1}.tar.xz
	mv boot-${1}.tar.xz ../../
	rm -rf *
	popd
	# NOOBS modifications, i.e. future 'health' script would be in .
	echo -e "Creating root tarball"
	echo "noobs" > vendor
	tar -cf - * | xz -9 -c - > root-${1}.tar.xz
	mv root-${1}.tar.xz ../
	popd
	rm -rf output
}
echo -e "Building NOOBS filesystem image"
build_fs_image "$1"
echo -e "Build completed"
