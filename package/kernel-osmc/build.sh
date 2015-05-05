# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh
test $1 == rbp1 && VERSION="3.18.11" && REV="2"
test $1 == rbp2 && VERSION="3.18.11" && REV="2"
test $1 == vero && VERSION="3.14.37" && REV="8"
if [ $1 == "rbp1" ] || [ $1 == "rbp2" ]
then
	if [ -z $VERSION ]; then echo "Don't have a defined kernel version for this target!" && exit 1; fi
	SOURCE_LINUX="https://www.kernel.org/pub/linux/kernel/v3.x/linux-${VERSION}.tar.xz"
fi
if [ $1 == "vero" ]; then SOURCE_LINUX="https://github.com/osmc/vero-linux/archive/master.tar.gz"; fi
pull_source "${SOURCE_LINUX}" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "kernel-osmc"
build_return=$?
if [ $build_return == 99 ]
then
	echo -e "Building Linux kernel"
	make clean
	sed '/Package/d' -i files/DEBIAN/control
	sed '/Depends/d' -i files/DEBIAN/control
	update_sources
	handle_dep "kernel-package"
	handle_dep "liblz4-tool"
	handle_dep "cpio"
	handle_dep "bison"
	handle_dep "flex"
	echo "maintainer := Sam G Nazarko
	email := email@samnazarko.co.uk
	priority := High" >/etc/kernel-pkg.conf
	JOBS=$(if [ ! -f /proc/cpuinfo ]; then mount -t proc proc /proc; fi; cat /proc/cpuinfo | grep processor | wc -l && umount /proc/ >/dev/null 2>&1)
	pushd src/*linux*
	if [ "$1" == "rbp1" ] || [ "$1" == "rbp2" ]
	then
		install_patch "../../patches" "rbp"
		# have to do this after, because upstream brings its own rtl8192cu in!
		rm -rf drivers/net/wireless/rtl8192cu
		mv rtl8192cu-new drivers/net/wireless/rtl8192cu
	fi
	# Set up DTC
	if [ "$1" == "rbp1" ] || [ "$1" == "rbp2" ]
	then
		pushd dtc-overlays
		$BUILD
		popd
		DTC=$(pwd)"/dtc-overlays/dtc"
	else
		$BUILD scripts
		DTC=$(pwd)"/scripts/dtc/dtc"
	fi
	install_patch "../../patches" "${1}"
	make-kpkg --stem $1 kernel_image --append-to-version -${REV}-osmc --jobs $JOBS --revision $REV
	if [ $? != 0 ]; then echo "Building kernel image package failed" && exit 1; fi
	make-kpkg --stem $1 kernel_headers --append-to-version -${REV}-osmc --jobs $JOBS --revision $REV
	if [ $? != 0 ]; then echo "Building kernel headers package failed" && exit 1; fi
	make-kpkg --stem $1 kernel_source --append-to-version -${REV}-osmc --jobs $JOBS --revision $REV
	if [ $? != 0 ]; then echo "Building kernel source package failed" && exit 1; fi
	# Make modules directory
	mkdir -p ../../files-image/lib/modules/${VERSION}-${REV}-osmc/kernel/drivers
	if [ "$1" == "rbp1" ] || [ "$1" == "rbp2" ]; then mkdir -p ../../files-image/boot/dtb-${VERSION}-${REV}-osmc/overlays; fi
	if [ "$1" == "vero" ]; then mkdir -p ../../files-image/boot/dtb-${VERSION}-${REV}-osmc; fi
	if [ "$1" == "rbp1" ]
	then
		make bcm2708-rpi-b.dtb
		make bcm2708-rpi-b-plus.dtb
	fi
	if [ "$1" == "rbp2" ]; then make bcm2709-rpi-2-b.dtb; fi
	if [ "$1" == "rbp1" ] || [ "$1" == "rbp2" ]
	then
		mv arch/arm/boot/dts/*.dtb ../../files-image/boot/dtb-${VERSION}-${REV}-osmc/
		overlays=( "hifiberry-dac-overlay" "hifiberry-dacplus-overlay" "hifiberry-digi-overlay" "iqaudio-dac-overlay" "iqaudio-dacplus-overlay" "rpi-dac-overlay" "lirc-rpi-overlay" "w1-gpio-overlay" "w1-gpio-pullup-overlay" "hy28a-overlay" "hy28b-overlay" "piscreen-overlay" "rpi-display-overlay" "spi-bcm2835-overlay" )
		pushd arch/arm/boot/dts
		for dtb in ${overlays[@]}
		do
			echo Building DT overlay $dtb
			$DTC -@ -I dts -O dtb -o $dtb.dtb $dtb.dts
		done
		popd
		mv arch/arm/boot/dts/*-overlay.dtb ../../files-image/boot/dtb-${VERSION}-${REV}-osmc/overlays
	fi
	if [ "$1" == "vero" ]
	then
		make imx6dl-vero.dtb
		mv arch/arm/boot/dts/*.dtb ../../files-image/boot/dtb-${VERSION}-${REV}-osmc/
		popd
	fi
	# Add out of tree modules that lack a proper Kconfig and Makefile
	# Fix CPU architecture
	ARCH=$(arch | tr -d v7l | tr -d v6)
	export ARCH
		if [ "$1" == "rbp1" ] || [ "$1" == "rbp2" ]
		then
		# Build RTL8812AU module
		mv rtl8812au drivers/net/wireless/rtl8812au
		pushd drivers/net/wireless/rtl8812au
		$BUILD
		if [ $? != 0 ]; then echo "Building kernel module failed" && exit 1; fi
		popd
		mkdir -p ../../files-image/lib/modules/${VERSION}-${REV}-osmc/kernel/drivers/net/wireless/
		cp drivers/net/wireless/rtl8812au/8812au.ko ../../files-image/lib/modules/${VERSION}-${REV}-osmc/kernel/drivers/net/wireless/
		fi
		if [ "$1" == "rbp1" ] || [ "$1" == "rbp2" ]
		then
		# Build RTL8192CU module
		# We already moved in place to satisfy Pi deps
		if [ "$1" != "rbp1" ] && [ "$1" != "rbp2" ]; then mv rtl8192cu-new drivers/net/wireless/rtl8192cu; fi
		pushd drivers/net/wireless/rtl8192cu
		$BUILD
		if [ $? != 0 ]; then echo "Building kernel module failed" && exit 1; fi
		popd
		mkdir -p ../../files-image/lib/modules/${VERSION}-${REV}-osmc/kernel/drivers/net/wireless/
		cp drivers/net/wireless/rtl8192cu/8192cu.ko ../../files-image/lib/modules/${VERSION}-${REV}-osmc/kernel/drivers/net/wireless/
		fi
	# Unset architecture
	ARCH=$(arch)
	export ARCH
	# Disassemble kernel package to add device tree overlays, additional out of tree modules etc
	mv src/${1}-image*.deb .
	dpkg -x ${1}-image*.deb files-image/
	dpkg-deb -e ${1}-image*.deb files-image/DEBIAN
	rm ${1}-image*.deb
	dpkg -b files-image ${1}-image-osmc.deb
	echo "Package: ${1}-kernel-osmc" >> files/DEBIAN/control
	echo "Depends: ${1}-image-${VERSION}-${REV}-osmc" >> files/DEBIAN/control
	fix_arch_ctl "files/DEBIAN/control"
	dpkg -b files/ kernel-${1}-osmc.deb
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
