# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

makedirnb()
{
  # Git doesn't allow empty folders. Do at runtime.
  if [ ! -d ${1} ]; then mkdir ${1}; fi
}

# Build in native environment
pull_source "https://www.kernel.org/pub/linux/kernel/v2.6/longterm/v2.6.32/linux-2.6.32.65.tar.xz" "$(pwd)/src/linux"
pull_source "http://busybox.net/downloads/busybox-1.23.0.tar.bz2" "$(pwd)/src/busybox"
pull_source "https://www.kernel.org/pub/linux/utils/kernel/kexec/kexec-tools-2.0.8.tar.xz" "$(pwd)/src/kexec-tools"
build_in_env "${1}" $(pwd) "atv-bootloader-osmc"

build_return=$?
if [ $build_return == 99 ]
then
	echo -e "Building package atv-bootloader"
	mkdir -p src/initrd
	out=$(pwd)/files
	make clean
	update_sources
	handle_dep "atv-darwin-cross-osmc"
	handle_dep "cpio"
	# Build Linux kernel
	cp .config src/linux/linux-*/
	pushd src/linux/linux-*
	$BUILD
	mv arch/x86/boot/vmlinux.bin ../../bootloader/vmlinuz
	make INSTALL_MOD_PATH="../../initrd" modules_install
	popd
	# Build BusyBox
	pushd src/busybox/busybox-*
	make defconfig
	CFLAGS="-static" $BUILD
	cp busybox ../../initrd
	popd
	# Build kexec-tools
	pushd src/kexec-tools/kexec-tools-*
	CFLAGS="-static" ./configure --prefix=/usr
	$BUILD
	cp build/sbin/kexec ../../initrd
	popd
	# Build initrd
	cp init src/initrd/
	chmod +x src/initrd/init
	pushd src/initrd
	makedirnb "proc"
	makedirnb "bin"
	makedirnb "tmp"
	makedirnb "usr"
	makedirnb "var"
	makedirnb "run"
	makedirnb "root"
	makedirnb "sys"
	makedirnb "lib"
	makedirnb "opt"
	makedirnb "home"
	makedirnb "lib64"
	makedirnb "mnt"
	makedirnb "selinux"
	makedirnb "boot"
	makedirnb "dev"
	mv busybox bin/
	mv kexec bin/
	pushd bin
	ln -s busybox mount
	ln -s busybox umount
	ln -s busybox sh
	ln -s busybox echo
	ln -s busybox \[
	ln -s busybox \[\[
	popd
	mknod dev/console c 5 1
	mknod dev/null c 1 3
	mknod dev/zero c 1 5
	# Internal and external disks without udev
	mknod dev/sda b 8 0
	mknod dev/sda1 b 8 1
	mknod dev/sdb b 8 0
	mknod dev/sdb1 b 8 0
	find . | cpio -H newc -o | gzip >../bootloader/initrd.gz
	popd
	# Build mach_kernel
	pushd src/bootloader
	make
	popd
	cp src/bootloader/mach_kernel files/boot/
	# Build Deb package
	dpkg -b files/ atv-bootloader-osmc.deb
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
