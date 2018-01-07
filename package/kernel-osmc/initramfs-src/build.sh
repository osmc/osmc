#!/bin/bash

# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

pushd ../ && . ../common.sh && popd

. VERSIONS

if [ -z "$1" ]; then echo -e "No target defined" && exit 1; fi
if [ -z "$2" ]; then echo -e "No device defined" && exit 1; fi
if [ "$1" == "cpio" ]
then
    ischroot
    chroot_result=$?
    if [ "$chroot_result" -eq 0 ] || [ "$chroot_result" -eq 1 ]
    then
        echo -e "Initramfs must be built within an OSMC chroot toolchain" && exit 1
    fi
fi
echo "Building initramfs for target ${1}"
make clean
update_sources
handle_dep "autoconf"
if [ "$1" == "cpio" ]
then
	rm -f *.tar.*
	#rm -rf busybox >/dev/null 2>&1
        #rm -rf e2fsprogs >/dev/null 2>&1
	handle_dep "cpio"
	handle_dep "wget" # Hack for poor man's pull_source
	handle_dep "ca-certificates" # kernel.org redirects to HTTPS
	# Use wget to get resources, as pull_source not compatible in chroot. Do not use in production
	wget "https://busybox.net/downloads/busybox-${BUSYBOX_VERSION}.tar.bz2"
	if [ $? != 0 ]; then echo "Could not get busybox sources" && exit 1; fi
	mkdir -p $(pwd)/busybox
	tar -xvf busybox-${BUSYBOX_VERSION}.tar.bz2 -C "$(pwd)/busybox"
	wget "https://www.kernel.org/pub/linux/kernel/people/tytso/e2fsprogs/v${E2FSPROGS_VERSION}/e2fsprogs-${E2FSPROGS_VERSION}.tar.gz"
	if [ $? != 0 ]; then echo "Could not get e2fsprogs sources" && exit 1; fi
	mkdir -p $(pwd)/e2fsprogs
	tar -xvf e2fsprogs-${E2FSPROGS_VERSION}.tar.gz -C "$(pwd)/e2fsprogs"
	if [ "$2" == "vero2" ] || [ "$2" == "vero364" ]
	then
            # LVM support
	    wget "https://mirrors.kernel.org/sourceware/lvm2/LVM2.${LVM_VERSION}.tgz"
	    if [ $? != 0 ]; then echo "Could not get LVM sources" && exit 1; fi
	    mkdir -p $(pwd)/lvm2
	    tar -xzvf LVM2.${LVM_VERSION}.tgz -C "$(pwd)/lvm2"
	fi
fi
echo "Compiling busybox"
pushd busybox/busybox-${BUSYBOX_VERSION}
cp ../../busybox.config .config
$BUILD
if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
popd
echo "Compiling e2fsprogs"
pushd e2fsprogs/e2fsprogs-${E2FSPROGS_VERSION}
# NB: can't build static because of pthread, which is part of glibc.
./configure --prefix=/usr --sysconfdir=/etc
$BUILD
if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
popd
if [ "$2" == "vero2" ] || [ "$2" == "vero364" ]
then
    echo "Compiling LVM2"
    pushd lvm2/LVM2.${LVM_VERSION}
    if [ "$2" == "vero364" ]
    then
        ## configure too old for arm64
        patch -p1 < ../../vero3-add-arm64-support.patch
    fi
    ./configure --prefix=/usr
    $BUILD
    mkdir out
    $BUILD DESTDIR=$(pwd)/out install
    if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
    popd
fi
mkdir -p target/
mkdir -p target/usr/lib
mkdir -p target/lib
mkdir -p target/sbin
mkdir -p target/bin
mkdir -p target/proc
mkdir -p target/sys
mkdir -p target/tmp
mkdir -p target/var
mkdir -p target/etc
mkdir -p target/dev
mkdir -p target/run
if [ "$2" == "vero2" ] || [ "$2" == "vero364" ]
then
    mkdir -p target/usr/sbin
    mkdir -p target/etc/lvm
fi
mkdir -p target/usr/share/udhcpc
install -m 0755 e2fsprogs/e2fsprogs-${E2FSPROGS_VERSION}/e2fsck/e2fsck target/bin/e2fsck
install -m 0755 busybox/busybox-${BUSYBOX_VERSION}/busybox target/bin/busybox
install -m 0755 init target/init
install -m 0755 init.d/${2} target/init-device
if [ "$2" == "vero2" ] || [ "$2" == "vero364" ]
then
    cp -ar lvm-avnftl.conf target/etc/lvm/lvm.conf
    install -m 0755 lvm2/LVM2.${LVM_VERSION}/out/usr/sbin/lvchange target/usr/sbin/lvchange
fi
cp -ar udhcpc.script target/usr/share/udhcpc/default.script
cp -ar e2fsck.conf target/etc/e2fsck.conf
ln -s target/bin/e2fsck target/bin/fsck.ext4
ln -s target/bin/e2fsck target/bin/fsck.ext3
ln -s target/bin/e2fsck target/bin/fsck.ext2
ln -s /proc/mounts target/etc/mtab
mknod target/dev/console c 5 1
mknod target/dev/ttyS0 c 204 64
mknod target/dev/null c 1 3
mknod target/dev/tty c 5 0
for line in $(ldd target/bin/e2fsck); do if (echo $line | grep -q /lib); then cp $line target/lib; fi; done
for line in $(ldd target/bin/busybox); do if (echo $line | grep -q /lib); then cp $line target/lib; fi; done
if [ "$2" == "vero2" ] || [ "$2" == "vero364" ]
then
    for line in $(ldd target/usr/sbin/lvchange); do if (echo $line | grep -q /lib); then cp $line target/lib; fi; done
    # HACK HACK HACK. Fix when not 6.33AM
    cp lvm2/LVM2.${LVM_VERSION}/out/usr/lib/libdevmapper.so.* target/usr/lib
    # Double hack. Would normally overwrite if consistent, but catch libdevmapper being on system.
    rm -f target/lib/libdevmapper.so.*
    # Dependency of a dependency..
    if [ "$2" == "vero2" ]; then cp /lib/arm-linux-gnueabihf/libsepol.so.1 target/lib/; fi
    if [ "$2" == "vero364" ]; then cp /lib/aarch64-linux-gnu/libsepol.so.1 target/lib; fi
fi
if [ "$1" == "cpio" ]
then
    pushd target
    # cpio
    find . | cpio -H newc -o > initramfs.cpio
    cat initramfs.cpio | gzip > initramfs.gz
    mv initramfs.gz ../
fi
echo "Initramfs built successfully"
