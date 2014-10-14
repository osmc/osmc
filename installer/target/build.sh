# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

echo -e "Building target side installer"
BUILDROOT_VERSION="2014.05"
echo -e "Installing dependencies"
#Check if we updated APT-Cache in the Last half hour
if [ `stat --format=%Y /var/lib/apt/listchanges.db` -le $(( `date +%s` - 1800 )) ]; then 
    apt-get update
fi
apt-get -y install build-essential rsync texinfo libncurses-dev whois unzip git subversion bc
#Check if already downloaded the buildroot
if [ ! -e "buildroot-${BUILDROOT_VERSION}.tar.gz" ] ; then
 wget http://buildroot.uclibc.org/downloads/buildroot-${BUILDROOT_VERSION}.tar.gz -O buildroot-${BUILDROOT_VERSION}.tar.gz
fi
if [ $? != 0 ]; then echo "Download failed" && exit 1; fi
tar -xzvf buildroot-${BUILDROOT_VERSION}.tar.gz
if [ $? != 0 ]; then echo "Extraction failed" && exit 1; fi
cp patches/* buildroot-${BUILDROOT_VERSION}
pushd buildroot-${BUILDROOT_VERSION}
for file in $(ls *.patch)
do
patch -p1 < $file
done
test "$1" == rbp && make osmc_rbp_defconfig
make
if [ $? != 0 ]; then echo "Build failed" && exit 1; fi
popd
pushd buildroot-${BUILDROOT_VERSION}/output/images
if [ $1 == rbp ]
then
	echo "Packaging build for Pi"
	mv zImage rpi-firmware
fi
popd
echo Build completed
