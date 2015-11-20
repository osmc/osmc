#!/bin/bash

# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

pushd ../ && . ../common.sh && popd

if [ -z "$1" ]; then echo -e "No target defined" && exit 1; fi
if [ "$1" == "cpio" ] && ! ischroot; then echo -e "Initramfs must be built within an OSMC chroot toolchain" && exit 1; fi
echo "Building initramfs for target ${1}"
make clean
handle_dep "autoconf"
BUSYBOX_VERSION="1.24.1"
E2FSPROGS_VERSION="1.42.13"
pull_source "http://busybox.net/downloads/busybox-${BUSYBOX_VERSION}.tar.bz2" "$(pwd)/busybox"
pull_source "http://www.kernel.org/pub/linux/kernel/people/tytso/e2fsprogs/v${E2FSPROGS_VERSION}/e2fsprogs-${E2FSPROGS_VERSION}.tar.gz" "$(pwd)/e2fsprogs"
echo "Compiling busybox"
pushd busybox/busybox-${BUSYBOX_VERSION}
cp ../../busybox.config .config
$BUILD
if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
popd
echo "Compiling e2fsprogs"
pushd e2fsprogs/e2fsprogs-${E2FSPROGS_VERSION}
# NB: can't build static because of pthread, which is part of glibc.
./configure --prefix=/usr
$BUILD
if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
popd
mkdir -p target/
mkdir -p target/lib
mkdir -p target/bin
mkdir -p target/proc
mkdir -p target/sys
mkdir -p target/tmp
mkdir -p target/var
mkdir -p target/etc
mkdir -p target/dev
ln -s target/bin/e2fsck target/bin/fsck.ext4
ln -s target/bin/e2fsck target/bin/fsck.ext3
ln -s target/bin/e2fsck target/bin/fsck.ext2
mknod target/dev/console c 5 1
mknod target/dev/ttyS0 c 204 64
mknod target/dev/null c 1 3
mknod target/dev/tty c 5 0
cp e2fsprogs/e2fsprogs-${E2FSPROGS_VERSION}/e2fsck/e2fsck target/bin
cp busybox/busybox-${BUSYBOX_VERSION}/busybox target/bin
cp init.sh target/init
chmod +x target/init
for line in $(ldd target/bin/e2fsck); do if (echo $line | grep -q /lib); then cp $line target/lib; fi; done
for line in $(ldd target/bin/busybox); do if (echo $line | grep -q /lib); then cp $line target/lib; fi; done
if [ "$1" == "cpio" ]
then
pushd target
# cpio
find . | cpio -H newc -o > initramfs.cpio
cat initramfs.cpio | gzip > initramfs.gz
mv initramfs.gz ../
fi
echo "Initramfs built successfully"
