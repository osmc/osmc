# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh
test $1 == rbp && VERSION="3.18.5" && REV="2"
if [ -z $VERSION ]; then echo "Don't have a defined kernel version for this target!" && exit 1; fi
pull_source "https://www.kernel.org/pub/linux/kernel/v3.x/linux-${VERSION}.tar.xz" "$(pwd)/src/"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "kernel-osmc"
if [ $? == 0 ]
then
	echo -e "Building Linux kernel"
	make clean
	sed '/Package/d' -i files/DEBIAN/control
	sed '/Depends/d' -i files/DEBIAN/control
	update_sources
	handle_dep "kernel-package"
	handle_dep "liblz4-tool"
	if [ "$1" != "rbp" ]
	then
		handle_dep "device-tree-compiler"
	else
		handle_dep "rbp-device-tree-compiler-osmc"
	fi
	echo "maintainer := Sam G Nazarko
	email := email@samnazarko.co.uk
	priority := High" >/etc/kernel-pkg.conf
	JOBS=$(if [ ! -f /proc/cpuinfo ]; then mount -t proc proc /proc; fi; cat /proc/cpuinfo | grep processor | wc -l && umount /proc/ >/dev/null 2>&1)
	pushd src/linux-*
	install_patch "../../patches" "all"
	test "$1" == "rbp" && install_patch "../../patches" "rbp"
	make-kpkg --stem $1 kernel_image --append-to-version -${REV}-osmc --jobs $JOBS --revision $REV
	if [ $? != 0 ]; then echo "Building kernel image package failed" && exit 1; fi
	make-kpkg --stem $1 kernel_headers --append-to-version -${REV}-osmc --jobs $JOBS --revision $REV
	if [ $? != 0 ]; then echo "Building kernel headers package failed" && exit 1; fi
	if [ "$1" == "rbp" ]
	then
		mkdir -p ../../files-image/boot/dtb-${VERSION}-${REV}-osmc/overlays
		make bcm2708-rpi-b.dtb
		make bcm2708-rpi-b-plus.dtb
		mv arch/arm/boot/dts/*.dtb ../../files-image/boot/dtb-${VERSION}-${REV}-osmc
		overlays="hifiberry-dac-overlay
		hifiberry-dacplus-overlay
		hifiberry-digi-overlay
		iqaudio-dac-overlay
		iqaudio-dacplus-overlay
		lirc-rpi-overlay
		w1-gpio-overlay
		w1-gpio-pullup-overlay"
		pushd arch/arm/boot/dts
		for dtb in $overlays
		do
			dtc -@ -I dts -O dtb -o $dtb.dtb $dtb.dts
		done
		popd
		mv arch/arm/boot/dts/*-overlay.dtb ../../files-image/boot/dtb-${VERSION}-${REV}-osmc/overlays
	fi
	popd
	if [ "$1" == "rbp" ]
	then
		# Disassemble kernel package to add overlays
		mv src/${1}-image*.deb .
		dpkg -x ${1}-image*.deb files-image/
		dpkg-deb -e ${1}-image*.deb files-image/DEBIAN
		rm ${1}-image*.deb
		dpkg -b files-image ${1}-image-osmc.deb
	fi
	echo "Package: ${1}-kernel-osmc" >> files/DEBIAN/control
	echo "Depends: ${1}-image-${VERSION}-${REV}-osmc" >> files/DEBIAN/control
	fix_arch_ctl "files/DEBIAN/control"
	dpkg -b files/ kernel-${1}-osmc.deb
fi
teardown_env "${1}"
