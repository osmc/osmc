# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../../scripts/common.sh

echo -e "Building target side installer"
BUILDROOT_VERSION="2018.11"

echo -e "Installing dependencies"
update_sources
verify_action
packages="build-essential
rsync
texinfo
libncurses5-dev
whois
bc
kpartx
dosfstools
parted
cpio
python3
python-is-python3
bison
flex
libssl-dev"

for package in $packages
do
	install_package $package
	verify_action
done

SIGN_KERNEL=0

if [ "$1" == "vero5" ]; then SIGN_KERNEL=1; fi

if [ -z "$PROVISION" ]
then
	PROVISION=0
else
	PROVISION=1
fi

if [ "$SIGN_KERNEL" -eq 1 ]
        then
		SIG_KEYS_DIR="/etc/osmc/keys"
		if [ ! -d $SIG_KEYS_DIR ]; then echo "Missing files needed for encrypting kernel image" && exit 1; fi
        fi

pull_source "https://buildroot.uclibc.org/downloads/buildroot-${BUILDROOT_VERSION}.tar.gz" "."
verify_action
pushd buildroot-${BUILDROOT_VERSION}
install_patch "../patches" "all"
install_patch "../patches" "$1"
if [ "$SIGN_KERNEL" -eq 1 ]
then
	install_patch "../patches" "signed-${1}"
fi
if [ "$PROVISION" -eq 1 ]
then
       install_patch "../patches" "provision-${1}"
       sed s/BR2_PACKAGE_OSMC/BR2_PACKAGE_OSMCPROVISION/ -i configs/osmc_defconfig
fi
if [ "$1" == "rbp2" ] || [ "$1" == "rbp4" ]
then
	install_patch "../patches" "rbp"
	sed s/rpi-firmware/rpi-firmware-osmc/ -i package/Config.in # Use our own firmware package
	echo "dwc_otg.fiq_fix_enable=1 sdhci-bcm2708.sync_after_dma=0 dwc_otg.lpm_enable=0 console=tty1 root=/dev/ram0 quiet init=/init loglevel=2 osmcdev=${1}" > package/rpi-firmware-osmc/cmdline.txt
fi
export FORCE_UNSAFE_CONFIGURE=1 # needed for tar package creation
make osmc_defconfig || make osmc_defconfig # Fixes buildroot 2018.x bug
make
if [ $? != 0 ]; then echo "Build failed" && exit 1; fi
popd
pushd buildroot-${BUILDROOT_VERSION}/output/images
if [ -f ../../../filesystem.tar.xz ] || [ "$PROVISION" -eq 1 ]
then
    echo -e "Using local filesystem or filesystem is not needed"
else
    echo -e "Downloading latest filesystem"
    date=$(date +%Y%m%d)
    count=150
    while [ $count -gt 0 ]; do wget --spider -q ${DOWNLOAD_URL}/filesystems/osmc-${1}-filesystem-${date}.tar.xz
           if [ "$?" -eq 0 ]; then
	        wget ${DOWNLOAD_URL}/filesystems/osmc-${1}-filesystem-${date}.tar.xz -O $(pwd)/../../../filesystem.tar.xz
		wget ${DOWNLOAD_URL}/filesystems/osmc-${1}-filesystem-${date}.md5 -O $(pwd)/../../../filesystem.md5
                break
           fi
           date=$(date +%Y%m%d --date "yesterday $date")
           let count=count-1
    done
fi
if [ ! -f ../../../filesystem.tar.xz ] && [ "$PROVISION" -ne 1 ]; then echo -e "No filesystem available for target" && exit 1; fi
if [ "$1" == "rbp2" ] || [ "$1" == "rbp4" ] || [ "$1" == "vero3" ] || [ "$1" == "vero5" ] && [ "$PROVISION" -ne 1 ]
then
	echo -e "Building disk image"
        size=320
        date=$(date +%Y%m%d)
	dd if=/dev/zero of=OSMC_TGT_${1}_${date}.img bs=1M count=${size} conv=sparse
	parted -s OSMC_TGT_${1}_${date}.img mklabel msdos
	parted -s OSMC_TGT_${1}_${date}.img mkpart primary fat32 4Mib 100%
	kpartx -s -a OSMC_TGT_${1}_${date}.img
	/sbin/partprobe
	mkfs.vfat -F32 /dev/mapper/loop*p1
	fatlabel /dev/mapper/loop*p1 OSMCInstall
	mount /dev/mapper/loop*p1 /mnt
fi
if [ "$1" == "rbp2" ] || [ "$1" == "rbp4" ]
then
	echo -e "Installing Pi files"
	mv zImage /mnt/kernel.img
	mv INSTALLER/* /mnt
	mv *.dtb /mnt
	mv overlays /mnt
	if [ "$1" == "rbp4" ]
	then
		rm /mnt/*rpi-b*.dtb
		rm /mnt/*rpi-*3*.dtb
		rm /mnt/*rpi-*2*.dtb
		rm /mnt/bcm2835*.dtb
		rm /mnt/bcm2708*.dtb
	fi
	if [ "$1" == "rbp2" ]
	then
		rm /mnt/bcm2711-rpi-*.dtb
	fi
fi
if [ "$1" == "vero3" ]
then
	echo -e "Installing Vero 3 files"
	DTB_FILE="multi.dtb"
	KERNEL_FILE="kernel.img"
	../.././output/build/linux-custom/scripts/multidtb/multidtb -o $DTB_FILE --dtc-path $(pwd)/../../output/build/linux-custom/scripts/dtc/ $(pwd)/../../output/build/linux-custom/arch/arm64/boot/dts/amlogic --verbose --page-size 2048
        if [ "$SIGN_KERNEL" -eq 1 ]
        then
            DTB_FILE="multi.dtb.encrypted"
	    KERNEL_FILE="kernel.img.encrypted"
	    ../.././output/build/linux-custom/scripts/mkbootimg --kernel Image.gz --pagesize 2048 --header_version 1 --base 0x0 --kernel_offset 0x1080000 --ramdisk rootfs.cpio.gz --second multi.dtb --output kernel.img
	    ../.././output/build/linux-custom/scripts/sign-kernel-boot.sh --sign-kernel --key-dir $SIG_KEYS_DIR --input multi.dtb --output multi.dtb.encrypted
	    ../.././output/build/linux-custom/scripts/sign-kernel-boot.sh --sign-kernel --key-dir $SIG_KEYS_DIR --input kernel.img --output kernel.img.encrypted
	else
            ../../output/build/linux-custom/scripts/mkbootimg --kernel Image.gz --base 0x0 --kernel_offset 0x1080000 --ramdisk rootfs.cpio.gz --second multi.dtb --output kernel.img
	fi

        if [ "$PROVISION" -ne 1 ]
        then
           cp $DTB_FILE /mnt/dtb.img
           cp $KERNEL_FILE /mnt/kernel.img
        fi
fi
if [ "$1" == "vero5" ]
then
        echo -e "Installing Vero 5 files"
        DTB_FILE="multi.dtb"
        KERNEL_FILE="kernel.img"
        ../.././output/build/linux-custom/scripts/multidtb/multidtb -o $DTB_FILE --dtc-path $(pwd)/../../output/build/linux-custom/scripts/dtc/ $(pwd)/../../output/build/linux-custom/arch/arm64/boot/dts/amlogic --verbose --page-size 2048
        if [ "$SIGN_KERNEL" -eq 1 ]
        then
            DTB_FILE="multi.dtb.encrypted"
            KERNEL_FILE="kernel.img.encrypted"
            ../.././output/build/linux-custom/scripts/mkbootimg --kernel Image.gz --pagesize 2048 --header_version 1 --base 0x0 --kernel_offset 0x1080000 --ramdisk rootfs.cpio.gz --second multi.dtb --output kernel.img
            ../.././output/build/linux-custom/scripts/sign-kernel-boot.sh --sign-kernel --key-dir $SIG_KEYS_DIR --input multi.dtb --output multi.dtb.encrypted
            ../.././output/build/linux-custom/scripts/sign-kernel-boot.sh --sign-kernel --key-dir $SIG_KEYS_DIR --input kernel.img --output kernel.img.encrypted
        else
            ../../output/build/linux-custom/scripts/mkbootimg --kernel Image.gz --base 0x0 --kernel_offset 0x1080000 --ramdisk rootfs.cpio.gz --second multi.dtb --output kernel.img
        fi

        if [ "$PROVISION" -ne 1 ]
        then
           cp $DTB_FILE /mnt/dtb.img
           cp $KERNEL_FILE /mnt/kernel.img
        fi
fi
if [ "$PROVISION" -ne 1 ]
then
    mv $(pwd)/../../../filesystem.tar.xz /mnt/
    umount /mnt
    sync
    kpartx -d OSMC_TGT_${1}_${date}.img
    echo -e "Compressing image"
    gzip OSMC_TGT_${1}_${date}.img
    md5sum OSMC_TGT_${1}_${date}.img.gz > OSMC_TGT_${1}_${date}.md5
    popd
    echo -e "Build completed"
fi
