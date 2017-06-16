# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

# initramfs flags

INITRAMFS_BUILD=1
INITRAMFS_EMBED=2
INITRAMFS_NOBUILD=4

. ../common.sh
test $1 == rbp1 && VERSION="4.9.29" && REV="10" && FLAGS_INITRAMFS=$(($INITRAMFS_BUILD + $INITRAMFS_EMBED)) && IMG_TYPE="zImage"
test $1 == rbp2 && VERSION="4.9.29" && REV="8" && FLAGS_INITRAMFS=$(($INITRAMFS_BUILD + $INITRAMFS_EMBED)) && IMG_TYPE="zImage"
test $1 == vero && VERSION="4.4.0" && REV="13" && FLAGS_INITRAMFS=$(($INITRAMFS_BUILD + $INITRAMFS_EMBED)) && IMG_TYPE="zImage"
test $1 == vero2 && VERSION="3.10.105" && REV="2" && FLAGS_INITRAMFS=$(($INITRAMFS_BUILD)) && IMG_TYPE="uImage"
test $1 == atv && VERSION="4.2.3" && REV="25" && FLAGS_INITRAMFS=$(($INITRAMFS_NOBUILD)) && IMG_TYPE="zImage"
test $1 == pc && VERSION="4.2.3" && REV="13" && FLAGS_INITRAMFS=$(($INITRAMFS_BUILD + $INITRAMFS_EMBED)) && IMG_TYPE="zImage"
test $1 == vero364 && VERSION="3.14.29" && REV="21" && FLAGS_INITRAMFS=$(($INITRAMFS_BUILD)) && IMG_TYPE="zImage"
if [ $1 == "rbp1" ] || [ $1 == "rbp2" ] || [ $1 == "atv" ] || [ $1 == "pc" ]
then
	if [ -z $VERSION ]; then echo "Don't have a defined kernel version for this target!" && exit 1; fi
	MAJOR=$(echo ${VERSION:0:1})
	DL_VERSION=${VERSION}
	VERSION_POINT_RLS=$(echo ${VERSION} | cut -d . -f 3)
	if [ "$VERSION_POINT_RLS" -eq 0 ]
	then
	    DL_VERSION=$(echo ${VERSION:0:3})
	fi
	SOURCE_LINUX="https://www.kernel.org/pub/linux/kernel/v${MAJOR}.x/linux-${DL_VERSION}.tar.xz"
fi
if [ $1 == "vero" ]; then SOURCE_LINUX="https://github.com/osmc/vero-linux/archive/master.tar.gz"; fi
if [ $1 == "vero2" ]; then SOURCE_LINUX="https://github.com/osmc/vero2-linux/archive/master.tar.gz"; fi
if [ $1 == "vero364" ]; then SOURCE_LINUX="http://github.com/osmc/vero3-linux/archive/master.tar.gz"; fi
pull_source "${SOURCE_LINUX}" "$(pwd)/src"
# We need to download busybox and e2fsprogs here because we run initramfs build within chroot and can't pull_source in a chroot
if ((($FLAGS_INITRAMFS & $INITRAMFS_NOBUILD) != $INITRAMFS_NOBUILD))
then
	. initramfs-src/VERSIONS
	pull_source "http://busybox.net/downloads/busybox-${BUSYBOX_VERSION}.tar.bz2" "$(pwd)/initramfs-src/busybox"
	pull_source "http://www.kernel.org/pub/linux/kernel/people/tytso/e2fsprogs/v${E2FSPROGS_VERSION}/e2fsprogs-${E2FSPROGS_VERSION}.tar.gz" "$(pwd)/initramfs-src/e2fsprogs"
        if [ "$1" == "vero2" ] || [ "$1" == "vero364" ]
	then
	    pull_source "https://mirrors.kernel.org/sourceware/lvm2/LVM2.${LVM_VERSION}.tgz" "$(pwd)/initramfs-src/lvm2"
	fi
fi
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
# Build in native environment
BUILD_OPTS=$BUILD_OPTION_DEFAULTS
BUILD_OPTS=$(($BUILD_OPTS - $BUILD_OPTION_USE_NOFP))
build_in_env "${1}" $(pwd) "kernel-osmc" "$BUILD_OPTS"
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
        if [ "$1" == "vero2" ]  || [ "$1" == "vero364" ]
        then
            handle_dep "u-boot-tools"
	    handle_dep "abootimg"
        fi
	export KPKG_MAINTAINER="Sam G Nazarko"
	export KPKG_EMAIL="email@samnazarko.co.uk"
	JOBS=$(if [ ! -f /proc/cpuinfo ]; then mount -t proc proc /proc; fi; cat /proc/cpuinfo | grep processor | wc -l && umount /proc/ >/dev/null 2>&1)
	pushd src/*linux*
	if [ "$1" == "rbp1" ] || [ "$1" == "rbp2" ]
	then
		install_patch "../../patches" "rbp"
	fi
	install_patch "../../patches" "${1}"
	# Set up DTC
	$BUILD scripts
	DTC=$(pwd)"/scripts/dtc/dtc"
	# Conver DTD to DTB
	if [ "$1" == "vero2" ]
	then
		$BUILD meson8b_vero2.dtd
		$BUILD meson8b_vero2.dtb
	fi
	if [ "$1" == "vero364" ]
	then
		# Debian Jessie has an ancient version of make-kpkg without arm64 definitions
		# So let's manually set up architectures.mk here and remove when Stretch arrives
		export kimage=vmlinuz
		export target=Image.gz
		export NEED_DIRECT_GZIP_IMAGE=YES
		export kimagesrc=arch/arm64/boot/Image
		export kimagedest=$(pwd)/vmlinuz
		export kelfimagedest=$(pwd)/vmlinux
		export KERNEL_ARCH=arm64
		$BUILD vero3_2g_16g.dtb
	fi
	# Initramfs time
	if ((($FLAGS_INITRAMFS & $INITRAMFS_NOBUILD) != $INITRAMFS_NOBUILD))
	then
		echo "This device requests an initramfs"
		pushd ../../initramfs-src
		DEVICE="$1" $BUILD kernel
		if [ $? != 0 ]; then echo "Building initramfs failed" && exit 1; fi
		popd
		if ((($FLAGS_INITRAMFS & $INITRAMFS_EMBED) == $INITRAMFS_EMBED))
		then
			echo "This device requests an initramfs to be embedded"
			export RAMFSDIR=$(pwd)/../../initramfs-src/target
		else
			echo "This device requests an initramfs to be built, but not embedded"
			pushd ../../initramfs-src/target
			find . | cpio -H newc -o | gzip - > ../initrd.img.gz
			popd
		fi
	fi
	if [ "$IMG_TYPE" == "zImage" ] || [ -z "$IMG_TYPE" ]; then make-kpkg --stem $1 kernel_image --append-to-version -${REV}-osmc --jobs $JOBS --revision $REV; fi
	if [ "$IMG_TYPE" == "uImage" ]; then make-kpkg --uimage --stem $1 kernel_image --append-to-version -${REV}-osmc --jobs $JOBS --revision $REV; fi
	if [ $? != 0 ]; then echo "Building kernel image package failed" && exit 1; fi
	make-kpkg --stem $1 kernel_headers --append-to-version -${REV}-osmc --jobs $JOBS --revision $REV
	if [ $? != 0 ]; then echo "Building kernel headers package failed" && exit 1; fi
	make-kpkg --stem $1 kernel_source --append-to-version -${REV}-osmc --jobs $JOBS --revision $REV
	if [ $? != 0 ]; then echo "Building kernel source package failed" && exit 1; fi
	# Make modules directory
	mkdir -p ../../files-image/lib/modules/${VERSION}-${REV}-osmc/kernel/drivers
	if [ "$1" == "rbp1" ] || [ "$1" == "rbp2" ]; then mkdir -p ../../files-image/boot/dtb-${VERSION}-${REV}-osmc/overlays; fi
	if [ "$1" == "vero" ]; then mkdir -p ../../files-image/boot/dtb-${VERSION}-${REV}-osmc; fi
	if [ "$1" == "vero2" ]; then mkdir -p ../../files-image/boot; fi
	if [ "$1" == "atv" ]; then mkdir -p ../../files-image/boot; fi
	if [ "$1" == "rbp1" ] || [ "$1" == "rbp2" ]
	then
		$BUILD dtbs
		mv arch/arm/boot/dts/*.dtb ../../files-image/boot/dtb-${VERSION}-${REV}-osmc/
		rename -v 's/\.dtbo$/\-overlay.dtb/' arch/arm/boot/dts/overlays/*.dtbo
		mv arch/arm/boot/dts/overlays/*.dtb ../../files-image/boot/dtb-${VERSION}-${REV}-osmc/overlays
		mv arch/arm/boot/dts/overlays/README ../../files-image/boot/dtb-${VERSION}-${REV}-osmc/overlays
	fi
	if [ "$1" == "vero" ]
	then
		make imx6dl-vero.dtb
		mv arch/arm/boot/dts/*.dtb ../../files-image/boot/dtb-${VERSION}-${REV}-osmc/
	fi
	if [ "$1" == "vero2" ]
	then
		# Special packaging for Android
		abootimg --create ../../files-image/boot/kernel-${VERSION}-${REV}-osmc.img -k arch/arm/boot/uImage -r ../../initramfs-src/initrd.img.gz -s arch/arm/boot/dts/amlogic/meson8b_vero2.dtb
		if [ $? != 0 ]; then echo "Building Android image for Vero 2 failed" && exit 1; fi
	fi
	if [ "$1" == "vero364" ]
        then
		mkdir -p ../../files-image/boot #hack
                # Special packaging for Android
                abootimg --create ../../files-image/boot/kernel-${VERSION}-${REV}-osmc.img -k arch/arm64/boot/Image.gz -r ../../initramfs-src/initrd.img.gz -s arch/arm64/boot/dts/amlogic/vero3_2g_16g.dtb -c "kerneladdr=0x1080000" -c "pagesize=0x800" -c "ramdiskaddr=0x1000000" -c "secondaddr=0xf00000" -c "tagsaddr=0x100"
                if [ $? != 0 ]; then echo "Building Android image for Vero 3 failed" && exit 1; fi
		# Hacks for lack of ARM64 native in kernel-package for Jessie
		cp -ar vmlinuz ../../files-image/boot/vmlinuz-${VERSION}-${REV}-osmc
		# Device tree for uploading to eMMC
		cp -ar arch/arm64/boot/dts/amlogic/vero3_2g_16g.dtb ../../files-image/boot/dtb-${VERSION}-${REV}-osmc.img
        fi
	# Add out of tree modules that lack a proper Kconfig and Makefile
	# Fix CPU architecture
	ARCH=$(arch)
    echo $ARCH | grep -q arm
    if [ $? == 0 ]
	then
	    ARCH=$(echo $ARCH | tr -d v7l | tr -d v6)
	fi
	if [ $ARCH == "i686" ]; then ARCH="i386"; fi
	export ARCH
		if [ "$1" == "rbp1" ] || [ "$1" == "rbp2" ] || [ "$1" == "atv" ] || [ "$1" == "vero" ] || [ "$1" == "vero2" ]
		then
		# Build RTL8812AU module
		pushd drivers/net/wireless/rtl8812au
		$BUILD
		if [ $? != 0 ]; then echo "Building kernel module failed" && exit 1; fi
		popd
		mkdir -p ../../files-image/lib/modules/${VERSION}-${REV}-osmc/kernel/drivers/net/wireless/
		strip --strip-unneeded drivers/net/wireless/rtl8812au/8812au.ko
		cp drivers/net/wireless/rtl8812au/8812au.ko ../../files-image/lib/modules/${VERSION}-${REV}-osmc/kernel/drivers/net/wireless/
		fi
		if [ "$1" == "rbp1" ] || [ "$1" == "rbp2" ] || [ "$1" == "atv" ] || [ "$1" == "vero" ]
		then
		# Build RTL8192CU module
		pushd drivers/net/wireless/rtl8192cu
		$BUILD
		if [ $? != 0 ]; then echo "Building kernel module failed" && exit 1; fi
		popd
		mkdir -p ../../files-image/lib/modules/${VERSION}-${REV}-osmc/kernel/drivers/net/wireless/
		strip --strip-unneeded drivers/net/wireless/rtl8192cu/8192cu.ko
		cp drivers/net/wireless/rtl8192cu/8192cu.ko ../../files-image/lib/modules/${VERSION}-${REV}-osmc/kernel/drivers/net/wireless/
		fi
		if [ "$1" == "rbp1" ] || [ "$1" == "rbp2" ] || [ "$1" == "atv" ] || [ "$1" == "vero" ]
		then
		# Build RTL8192DU model
		pushd drivers/net/wireless/rtl8192du
		$BUILD
		if [ $? != 0 ]; then echo -e "Building kernel module failed" && exit 1; fi
		popd
		mkdir -p ../../files-image/lib/modules/${VERSION}-${REV}-osmc/kernel/drivers/net/wireless/
		strip --strip-unneeded drivers/net/wireless/rtl8192du/8192du.ko
		cp drivers/net/wireless/rtl8192du/8192du.ko ../../files-image/lib/modules/${VERSION}-${REV}-osmc/kernel/drivers/net/wireless/
		fi
		if [ "$1" == "rbp1" ] || [ "$1" == "rbp2" ] || [ "$1" == "atv" ] || [ "$1" == "vero" ]
		then
		# Build RTL8192EU model
		pushd drivers/net/wireless/rtl8192eu
		$BUILD
		if [ $? != 0 ]; then echo -e "Building kernel module failed" && exit 1; fi
		popd
		mkdir -p ../../files-image/lib/modules/${VERSION}-${REV}-osmc/kernel/drivers/net/wireless/
		strip --strip-unneeded drivers/net/wireless/rtl8192eu/8192eu.ko
		cp drivers/net/wireless/rtl8192eu/8192eu.ko ../../files-image/lib/modules/${VERSION}-${REV}-osmc/kernel/drivers/net/wireless/
		fi
		if [ "$1" == "rbp1" ] || [ "$1" == "rbp2" ] || [ "$1" == "atv" ] || [ "$1" == "vero" ] || [ "$1" == "vero2" ]
		then
                # Build MT7610U model
                pushd drivers/net/wireless/mt7610u
                $BUILD
                if [ $? != 0 ]; then echo -e "Building kernel module failed" && exit 1; fi
                popd
                mkdir -p ../../files-image/lib/modules/${VERSION}-${REV}-osmc/kernel/drivers/net/wireless/
		strip --strip-unneeded drivers/net/wireless/mt7610u/os/linux/mt7610u_sta.ko
                cp drivers/net/wireless/mt7610u/os/linux/mt7610u_sta.ko ../../files-image/lib/modules/${VERSION}-${REV}-osmc/kernel/drivers/net/wireless/
                fi
		if [ "$1" == "atv" ]
		then
		# Build CrystalHD
		pushd drivers/staging/chd
		$BUILD
		if [ $? != 0 ]; then echo -e "Building kernel module failed" && exit 1; fi
		popd
		mkdir -p ../../files-image/lib/modules/${VERSION}-${REV}-osmc/kernel/drivers/staging/chd/
		strip --strip-unneeded drivers/staging/chd/crystalhd.ko
		cp drivers/staging/chd/crystalhd.ko ../../files-image/lib/modules/${VERSION}-${REV}-osmc/kernel/drivers/staging
		fi
		if [ "$1" == "atv" ]
		then
		# Build NVIDIA module
		pushd drivers/staging/nv-osmc
		$BUILD SYSSRC=$(pwd)/../../../
		if [ $? != 0 ]; then echo -e "Building kernel module failed" && exit 1; fi
		popd
		mkdir -p ../../files-image/lib/modules/${VERSION}-${REV}-osmc/kernel/drivers/staging/nv-osmc
		strip --strip-unneeded drivers/staging/nv-osmc/nvidia.ko
		cp drivers/staging/nv-osmc/nvidia.ko ../../files-image/lib/modules/${VERSION}-${REV}-osmc/kernel/drivers/staging
		fi
        # Build V4L2 drivers for Vero 4K
        if [ "$1" == "vero364" ]
        then
		export ARCH=arm64
		kernel_path=$(pwd)
                pushd media_build
                make untar
                cp -a "../drivers/amlogic/video_dev" "linux/drivers/media/"
                sed -i 's,common/,,g; s,"trace/,",g' $(find linux/drivers/media/video_dev/ -type f)
                sed -i 's,\$(CONFIG_V4L_AMLOGIC_VIDEO),m,g' "linux/drivers/media/video_dev/Makefile"
                echo "obj-y += video_dev/" >> "linux/drivers/media/Makefile"
                echo "source drivers/media/video_dev/Kconfig " >> "linux/drivers/media/Kconfig"
                cp -a "${kernel_path}/drivers/media/v4l2-core/videobuf-res.c" "linux/drivers/media/v4l2-core/"
                cp -a "${kernel_path}/include/media/videobuf-res.h" "linux/include/media/"
                echo "obj-m += videobuf-res.o" >> "linux/drivers/media/v4l2-core/Makefile"
                $BUILD VER=${VERSION} SRCDIR=$(pwd)/../
                popd
                mkdir -p ../../files-image/lib/modules/${VERSION}-${REV}-osmc/kernel/drivers/backport
		cp media_build/v4l/*.ko ../../files-image/lib/modules/${VERSION}-${REV}-osmc/kernel/drivers/backport
        fi
	# Unset architecture
	ARCH=$(arch)
	export ARCH
	popd
	# Move all of the Debian packages so they are where we would expect them
	mv src/${1}-*.deb .
	# Disassemble kernel image package to add device tree overlays, additional out of tree modules etc
	dpkg -x ${1}-image*.deb files-image/
	dpkg-deb -e ${1}-image*.deb files-image/DEBIAN
	rm ${1}-image*.deb
	dpkg_build files-image ${1}-image-${VERSION}-${REV}-osmc.deb
	# Disassemble kernel headers package to include full headers (upstream Debian bug...)
	if [ "$ARCH" == "armv7l" ]
	then
		mkdir -p files-headers/
		dpkg -x ${1}-headers*.deb files-headers/
		dpkg-deb -e ${1}-headers*.deb files-headers/DEBIAN
		rm ${1}-headers*.deb
		cp -ar src/*linux*/arch/arm/include/ files-headers/usr/src/*-headers-${VERSION}-${REV}-osmc/include
		dpkg_build files-headers ${1}-headers-${VERSION}-${REV}-osmc.deb
	fi
	echo "Package: ${1}-kernel-osmc" >> files/DEBIAN/control
	echo "Depends: ${1}-image-${VERSION}-${REV}-osmc" >> files/DEBIAN/control
	fix_arch_ctl "files/DEBIAN/control"
	dpkg_build files/ ${1}-kernel-${VERSION}-${REV}-osmc.deb
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
