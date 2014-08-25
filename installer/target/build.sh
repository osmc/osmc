# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

echo -e "Building target side installer"
BUILDROOT_VERSION="2014.05"
echo -e "Installing dependencies"
apt-get update
apt-get -y install build-essential rsync texinfo libncurses-dev whois unzip git subversion bc
wget http://buildroot.uclibc.org/downloads/buildroot-${BUILDROOT_VERSION}.tar.gz -O buildroot.tar.gz
if [ $? != 0 ]; then echo "Download failed" && exit 1; fi
tar -xzvf buildroot.tar.gz
if [ $? != 0 ]; then echo "Extraction failed" && exit 1; fi
cd buildroot-${BUILDROOT_VERSION}
cp patches/* buildroot-${BUILDROOT_VERSION}
pushd buildroot*/
for file in $(ls *.patch)
do
patch -p1 < $file
done
test "$1" == rbp && make osmc_rbp_defconfig
make
if [ $? != 0 ]; then echo "Build failed" && exit 1; fi
pushd buildroot-${BUILDROOT_VERSION}/output/images
if [ $1 == rbp ]
then
	echo "Packaging build for Pi"
	mv zImage rpi-firmware
fi
echo Build completed
