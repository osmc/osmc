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
	echo -e "Creating boot tarball"
	pushd boot
	UNC_TS_SIZE_BOOT=$(du -h --max-depth=0 . | awk {'print $1'} | tr -d 'M')
	tar -cf - * | xz -9 -c - > boot-${1}.tar.xz
	mv boot-${1}.tar.xz ../../
	rm -rf *
	popd
	# NOOBS modifications, i.e. future 'health' script would be in .
	echo -e "Creating root tarball"
	UNC_TS_SIZE_ROOT=$(du -h --max-depth=0 . | awk {'print $1'} | tr -d 'M')
	echo "noobs" > vendor
	tar -cf - * | xz -9 -c - > root-${1}.tar.xz
	mv root-${1}.tar.xz ../
	popd
	# Set uncompressed tarball sizes
	sed -e s/UNC_TS_SIZE_BOOT/${UNC_TS_SIZE_BOOT}/ -i partitions-pi$(echo ${1:3:4}).json
	sed -e s/UNC_TS_SIZE_ROOT/${UNC_TS_SIZE_ROOT}/ -i partitions-pi$(echo ${1:3:4}).json
	echo -e "Creating marketing tarball"
	tar -cf marketing.tar slides_vga slides
	rm -rf output
	rm -rf vga_slides
}
echo -e "Building NOOBS filesystem image"
make clean
build_fs_image "$1"
echo -e "Build completed"
