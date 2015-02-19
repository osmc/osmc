# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../../scripts/common.sh

function build_fs_image()
{
	echo -e "Downloading latest filesystem for RBP Version ${1}"
	date=$(date +%Y%m%d)
	count=150
	while [ $count -gt 0 ]; do wget --spider -q ${DOWNLOAD_URL}/filesystems/osmc-rbp${1}-filesystem-${date}.tar.xz
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
	tar -cf - * | xz -9 -c - > boot${1}.tar.xz
	mv boot${1}.tar.xz ../../
	rm *
	popd
	# NOOBS modifications, i.e. future 'health' script would be in ../
	tar -cf - * | xz -9 -c - > root${1}.tar.xz
	mv root${1}.tar.xz ../../
	popd
}
echo -e "Building NOOBS filesystem image"
build_fs_image "1" # Pi 1
build_fs_image "2" # Pi 2
echo -e "Build completed"
