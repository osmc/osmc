
# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

# initramfs flags

INITRAMFS_BUILD=1
INITRAMFS_EMBED=2
INITRAMFS_NOBUILD=4

sign_module () {

# Generic function to sign kernel module with key

if [ "$SIGN_KERNEL" -eq 0 ]; then
    echo "* Ignoring request to sign kernel module $1 as device does not request it"
    return
else
    echo "* Signing kernel module $1 as device requests it"
    scripts/sign-file sha256 certs/signing_key.pem certs/signing_key.x509 $1
fi
}

. ../common.sh
test $1 == rbp2 && VERSION="5.10.22" && REV="2" && FLAGS_INITRAMFS=$(($INITRAMFS_BUILD + $INITRAMFS_EMBED)) && IMG_TYPE="zImage" && SIGN_KERNEL=0
test $1 == rbp464 && VERSION="5.10.22" && REV="3" && FLAGS_INITRAMFS=$(($INITRAMFS_BUILD + $INITRAMFS_EMBED)) && IMG_TYPE="zImage" && SIGN_KERNEL=0
test $1 == pc && VERSION="4.2.3" && REV="16" && FLAGS_INITRAMFS=$(($INITRAMFS_BUILD + $INITRAMFS_EMBED)) && IMG_TYPE="zImage" && SIGN_KERNEL=0
test $1 == vero364 && VERSION="4.9.113" && REV="33" && FLAGS_INITRAMFS=$(($INITRAMFS_BUILD)) && IMG_TYPE="zImage" && SIGN_KERNEL=0
if [ $1 == "rbp2" ] || [ $1 == "rbp464" ] || [ $1 == "pc" ]
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
if [ $1 == "vero364" ]; then SOURCE_LINUX="https://github.com/osmc/vero3-linux/archive/osmc-openlinux-4.9.tar.gz"; fi
pull_source "${SOURCE_LINUX}" "$(pwd)/src"
# We need to download busybox and e2fsprogs here because we run initramfs build within chroot and can't pull_source in a chroot
if ((($FLAGS_INITRAMFS & $INITRAMFS_NOBUILD) != $INITRAMFS_NOBUILD))
then
	. initramfs-src/VERSIONS
	pull_source "https://busybox.net/downloads/busybox-${BUSYBOX_VERSION}.tar.bz2" "$(pwd)/initramfs-src/busybox"
	pull_source "https://www.kernel.org/pub/linux/kernel/people/tytso/e2fsprogs/v${E2FSPROGS_VERSION}/e2fsprogs-${E2FSPROGS_VERSION}.tar.gz" "$(pwd)/initramfs-src/e2fsprogs"
        if [ "$1" == "vero364" ]
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
	sed '/Package/d' -i files-image/DEBIAN/control
	sed '/Version/d' -i files-image/DEBIAN/control
        sed '/Package/d' -i files-headers/DEBIAN/control
        sed '/Version/d' -i files-headers/DEBIAN/control
        sed '/Package/d' -i files-debug/DEBIAN/control
        sed '/Version/d' -i files-debug/DEBIAN/control
        sed '/Package/d' -i files-source/DEBIAN/control
        sed '/Version/d' -i files-source/DEBIAN/control
	update_sources
	handle_dep "libssl-dev"
	handle_dep "liblz4-tool"
	handle_dep "cpio"
	handle_dep "bison"
	handle_dep "flex"
	handle_dep "rsync"
	handle_dep "openssl"
        if [ "$1" == "vero364" ]
        then
	    handle_dep "python"
	    handle_dep "xxd"
        fi
	if [ "$SIGN_KERNEL" -eq 1 ]
	then
		SIG_FILE_AES="/etc/osmc/kernelaes"
		SIG_FILE_AESIV="/etc/osmc/kernelaesiv"
		SIG_FILE_KERNELKEY="/etc/osmc/kernelkey.pem"
		if [ ! -f $SIG_FILE_AES ] || [ ! -f $SIG_FILES_AESIV ] || [ ! -f $SIG_FILE_KERNELKEY ]; then echo "Missing files needed for encrypting kernel image" && exit 1; fi
	fi
	JOBS=$(if [ ! -f /proc/cpuinfo ]; then mount -t proc proc /proc; fi; cat /proc/cpuinfo | grep processor | wc -l && umount /proc/ >/dev/null 2>&1)
	pushd src/*linux*
        # Add out of tree modules that lack a proper Kconfig and Makefile
        # Fix CPU architecture
        ARCH=$(arch)
        echo $ARCH | grep -q arm
        if [ $? == 0 ]
            then
                ARCH=$(echo $ARCH | tr -d v7l | tr -d v6)
        fi
        if [ $ARCH == "i686" ]; then ARCH="i386"; fi
        if [ "$1" == "vero364" ]; then ARCH=arm64; fi
        if [ "$1" == "rbp464" ]; then ARCH=arm64; fi
        export ARCH
	if [ "$1" == "rbp2" ] || [ "$1" == "rbp464" ]
	then
		install_patch "../../patches" "rbp"
	fi
	install_patch "../../patches" "${1}"
        if [ "$1" == "rbp2" ] || [ "$1" == "rbp464" ]
        then
                # We have to do this here separately because we need .config present first
                ./scripts/config --set-val CONFIG_ARM64_TLB_RANGE y
                ./scripts/config --set-val ARM64_PTR_AUTH y
                ./scripts/config --set-val CONFIG_KASAN n
                ./scripts/config --set-val CONFIG_KCOV n
        fi
        if [ "$SIGN_KERNEL" -eq 1 ] # This functionality is only supported on Vero for now. Needs fragment in place for support and decision on whether final keysig should be ephem or not depending on chain of trust
        then
		scripts/kconfig/merge_config.sh .config arch/${ARCH}/configs/enforce-kernel-module-signature.fragment
        fi
	# Set up DTC
	$BUILD scripts
	DTC=$(pwd)"/scripts/dtc/dtc"
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
	# Set versioning information for packaging
	sed "s/EXTRAVERSION =/EXTRAVERSION = -${REV}-osmc/" -i Makefile
	# Create contents of source package
	mkdir -p ../../files-source/usr/src/${1}-source-${VERSION}-${REV}-osmc/
	cp -ar * ../../files-source/usr/src/${1}-source-${VERSION}-${REV}-osmc/
	# Build the kernel
	$BUILD
        if [ $? != 0 ]; then echo "Building kernel image failed" && exit 1; fi
	# Build kernel modules
        $BUILD modules_install INSTALL_MOD_STRIP=1 INSTALL_MOD_PATH=../../files-image/
	# Hack: we don't need in-tree FW. Instead of patching the Makefile, rip it.
	rm -rf ../../files-image/lib/firmware/
        if [ $? != 0 ]; then echo "Building kernel modules failed" && exit 1; fi
	mkdir -p ../../files-image/boot
	# Copy config in to boot (files-image)
	cp .config ../../files-image/boot/config-${VERSION}-${REV}-osmc
	# Copy system map to boot (files-image)
        cp System.map ../../files-image/boot/System.map-${VERSION}-${REV}-osmc
	# Debug kernel
	mkdir -p ../../files-debug/var/osmc
	cp vmlinux ../../files-debug/var/osmc/${1}-debug-${VERSION}-${REV}-osmc
	# Install headers in to headers package
        $BUILD headers_install INSTALL_HDR_PATH=../../files-headers/usr/src/${1}-headers-${VERSION}-${REV}-osmc/
	if [ $? != 0 ]; then echo "Building kernel packages failed" && exit 1; fi
	# Make modules directory
	mkdir -p ../../files-image/lib/modules/${VERSION}-${REV}-osmc/kernel/drivers
	if [ "$1" == "rbp2" ] || [ "$1" == "rbp464" ]; then mkdir -p ../../files-image/boot/dtb-${VERSION}-${REV}-osmc/overlays; fi
        if [ "$1" == "rbp2" ] || [ "$1" == "rbp464" ]
        then
                $BUILD dtbs
                mv arch/arm/boot/dts/*.dtb ../../files-image/boot/dtb-${VERSION}-${REV}-osmc/ || true
                mv arch/arm64/boot/dts/broadcom/*.dtb ../../files-image/boot/dtb-${VERSION}-${REV}-osmc/ || true
                mv arch/arm*/boot/dts/overlays/*.dtbo ../../files-image/boot/dtb-${VERSION}-${REV}-osmc/overlays || true
                mv arch/arm/boot/dts/overlays/README ../../files-image/boot/dtb-${VERSION}-${REV}-osmc/overlays || true
                mv arch/arm/boot/dts/overlays/overlay_map.dtb ../../files-image/boot/dtb-${VERSION}-${REV}-osmc/overlays || true # This is a hack for Pi 4, because overlay_map.dtb isn't present in arm64 directory
                if [ "$1" == "rbp2" ]
                then
                        rm ../../files-image/boot/dtb-${VERSION}-${REV}-osmc/bcm2711-rpi-*.dtb
                fi
                if [ "$1" == "rbp464" ]
                then
                        rm ../../files-image/boot/dtb-${VERSION}-${REV}-osmc/*rpi-b*.dtb
                        rm ../../files-image/boot/dtb-${VERSION}-${REV}-osmc/*rpi-*3*.dtb
                        rm ../../files-image/boot/dtb-${VERSION}-${REV}-osmc/*rpi-*2*.dtb
                        rm ../../files-image/boot/dtb-${VERSION}-${REV}-osmc/bcm2835*.dtb
                        rm ../../files-image/boot/dtb-${VERSION}-${REV}-osmc/bcm2708*.dtb
               fi
        fi
	if [ "$1" == "vero364" ]
        then
                $BUILD vero3_2g_16g.dtb || $BUILD vero3_2g_16g.dtb
                $BUILD vero3plus_2g_16g.dtb || $BUILD vero3plus_2g_16g.dtb
		mkdir -p ../../files-image/boot #hack
                # Special packaging for Android
		./scripts/multidtb/multidtb -p scripts/dtc/ -o multi.dtb arch/arm64/boot/dts/amlogic --verbose --page-size 2048
		if [ $? != 0 ]; then echo "Packing DTB failed"; fi
		DTB_FILE="multi.dtb"
		if [ "$SIGN_KERNEL" -eq 1 ]
		then
			DTB_FILE="multi.dtb.encrypted"
			scripts/amlogic/stool/sign.sh --sign-kernel -i multi.dtb -k $SIG_FILE_KERNELKEY -a $SIG_FILE_AES --iv $SIG_FILE_AESIV -o multi.dtb.encrypted || true
			if [ $? != 0 ]; then echo "Signing Device Tree failed" && exit 1; fi
			scripts/mkbootimg --kernel arch/arm64/boot/Image.gz --pagesize 2048 --header_version 1 --base 0x0 --kernel_offset 0x1080000 --ramdisk ../../initramfs-src/initrd.img.gz --second multi.dtb --output kernel.img
			if [ $? != 0 ]; then echo "Creating boot image failed in secure mode" && exit 1; fi
			scripts/amlogic/stool/sign.sh --sign-kernel -i kernel.img -k $SIG_FILE_KERNELKEY -a $SIG_FILE_AES --iv $SIG_FILE_AESIV -o ../../files-image/boot/kernel-${VERSION}-${REV}-osmc.img || true
			if [ $? != 0 ]; then echo "Signing kernel image failed" && exit 1; fi
		else
			scripts/mkbootimg --kernel arch/arm64/boot/Image.gz --pagesize 2048 --header_version 1 --base 0x0 --kernel_offset 0x1080000 --ramdisk ../../initramfs-src/initrd.img.gz --second multi.dtb --output ../../files-image/boot/kernel-${VERSION}-${REV}-osmc.img
			if [ $? != 0 ]; then echo "Creating boot image failed in non-secure mode" && exit 1; fi

		fi
		# Hacks for lack of ARM64 native in kernel-package for Jessie
		# Device tree for uploading to eMMC
		cp -ar $DTB_FILE ../../files-image/boot/dtb-${VERSION}-${REV}-osmc.img
        fi
	if [ "$1" == "rbp464" ]
	then
		cp -ar arch/arm64/boot/Image ../../files-image/boot/vmlinuz-${VERSION}-${REV}-osmc
	fi
        if [ "$1" == "rbp2" ]
        then
                cp -ar arch/arm/boot/zImage ../../files-image/boot/vmlinuz-${VERSION}-${REV}-osmc
        fi
		if [ "$1" == "vero2" ]
		then
		# Build RTL8812AU module
		pushd drivers/net/wireless/rtl8812au
		$BUILD
		if [ $? != 0 ]; then echo "Building kernel module failed" && exit 1; fi
		popd
		mkdir -p ../../files-image/lib/modules/${VERSION}-${REV}-osmc/kernel/drivers/net/wireless/
		strip --strip-unneeded drivers/net/wireless/rtl8812au/*8812au.ko
		cp drivers/net/wireless/rtl8812au/*8812au.ko ../../files-image/lib/modules/${VERSION}-${REV}-osmc/kernel/drivers/net/wireless/
		fi
        if [ "$1" == "vero364" ]
	then
		# Build V4L2 modules for Vero 4K
		$BUILD M=drivers/osmc/media_modules CONFIG_AMLOGIC_MEDIA_VDEC_OSMC=m
		mkdir -p ../../files-image/lib/modules/${VERSION}-${REV}-osmc/kernel/drivers/osmc
		for file in $(find drivers/osmc/media_modules/ -name "*.ko"); do
		sign_module $file
		cp $file ../../files-image/lib/modules/${VERSION}-${REV}-osmc/kernel/drivers/osmc
		done
		# Build OpTEE modules for secureOSMC
		$BUILD M=drivers/osmc/secureosmc
		mkdir -p ../../files-image/lib/modules/${VERSION}-${REV}-osmc/kernel/drivers/osmc/secureosmc
		sign_module "drivers/osmc/secureosmc/optee/optee_armtz.ko"
		sign_module "drivers/osmc/secureosmc/optee.ko"
		cp drivers/osmc/secureosmc/optee/optee_armtz.ko ../../files-image/lib/modules/${VERSION}-${REV}-osmc/kernel/drivers/osmc/secureosmc
		cp drivers/osmc/secureosmc/optee.ko ../../files-image/lib/modules/${VERSION}-${REV}-osmc/kernel/drivers/osmc/secureosmc
	fi
	# Unset architecture
	ARCH=$(arch)
	export ARCH
	popd
	echo "Package: ${1}-image-${VERSION}-${REV}-osmc" >> files-image/DEBIAN/control
	echo "Version: ${VERSION}-${REV}-osmc" >> files-image/DEBIAN/control
        echo "Package: ${1}-headers-${VERSION}-${REV}-osmc" >> files-headers/DEBIAN/control
        echo "Version: ${VERSION}-${REV}-osmc" >> files-headers/DEBIAN/control
        echo "Package: ${1}-debug-${VERSION}-${REV}-osmc" >> files-debug/DEBIAN/control
        echo "Version: ${VERSION}-${REV}-osmc" >> files-debug/DEBIAN/control
        echo "Package: ${1}-source-${VERSION}-${REV}-osmc" >> files-source/DEBIAN/control
        echo "Version: ${VERSION}-${REV}-osmc" >> files-source/DEBIAN/control
	tee files-image/DEBIAN/postinst << EOF
#!/bin/sh
set -e

# Pass maintainer script parameters to hook scripts
export DEB_MAINT_PARAMS="\$*"

# Tell initramfs builder whether it's wanted
export INITRD=Yes

test -d /etc/kernel/postinst.d && run-parts --arg="${VERSION}-${REV}-osmc" --arg="/boot/vmlinuz-${VERSION}-${REV}-osmc" /etc/kernel/postinst.d

# OSMC -- make sure any out of tree modules are picked up properly
/sbin/depmod "${VERSION}-${REV}-osmc"

exit 0
EOF
        tee files-image/DEBIAN/postrm << EOF
#!/bin/sh
set -e

# Pass maintainer script parameters to hook scripts
export DEB_MAINT_PARAMS="\$*"

# Tell initramfs builder whether it's wanted
export INITRD=Yes

test -d /etc/kernel/postrm.d && run-parts --arg="${VERSION}-${REV}-osmc" --arg="/boot/vmlinuz-${VERSION}-${REV}-osmc" /etc/kernel/postrm.d
exit 0
EOF
        tee files-image/DEBIAN/preinst << EOF
#!/bin/sh
set -e

# Pass maintainer script parameters to hook scripts
export DEB_MAINT_PARAMS="\$*"

# Tell initramfs builder whether it's wanted
export INITRD=Yes

test -d /etc/kernel/preinst.d && run-parts --arg="${VERSION}-${REV}-osmc" --arg="/boot/vmlinuz-${VERSION}-${REV}-osmc" /etc/kernel/preinst.d
exit 0
EOF
        tee files-image/DEBIAN/prerm << EOF
#!/bin/sh
set -e

# Pass maintainer script parameters to hook scripts
export DEB_MAINT_PARAMS="\$*"

# Tell initramfs builder whether it's wanted
export INITRD=Yes

test -d /etc/kernel/prerm.d && run-parts --arg="${VERSION}-${REV}-osmc" --arg="/boot/vmlinuz-${VERSION}-${REV}-osmc" /etc/kernel/prerm.d
exit 0
EOF
	chmod +x files-image/DEBIAN/post*
	chmod +x files-image/DEBIAN/pre*
        fix_arch_ctl "files-image/DEBIAN/control"
        fix_arch_ctl "files-headers/DEBIAN/control"
        fix_arch_ctl "files-debug/DEBIAN/control"
	fix_arch_ctl "files-source/DEBIAN/control"
	dpkg_build files-image ${1}-image-${VERSION}-${REV}-osmc.deb
	dpkg_build files-headers ${1}-headers-${VERSION}-${REV}-osmc.deb
	dpkg_build files-debug ${1}-image-debug-${VERSION}-${REV}-osmc.deb
	dpkg_build files-source ${1}-source-${VERSION}-${REV}-osmc.deb
	echo "Package: ${1}-kernel-osmc" >> files/DEBIAN/control
	if [ "$1" == "vero364" ]
	then
		echo "Depends: ${1}-image-${VERSION}-${REV}-osmc, vero3-bootloader-osmc:armhf (>=1.0.0)" >> files/DEBIAN/control
	else
		echo "Depends: ${1}-image-${VERSION}-${REV}-osmc" >> files/DEBIAN/control
	fi
	fix_arch_ctl "files/DEBIAN/control"
	dpkg_build files/ ${1}-kernel-${VERSION}-${REV}-osmc.deb
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
