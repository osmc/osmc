# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../../scripts/common.sh

echo -e "Building target side installer"
BUILDROOT_VERSION="2014.05"

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
python
bison
flex"

if [ "$1" == "vero2" ]
then
   packages="u-boot-tools $packages"
fi

for package in $packages
do
	install_package $package
	verify_action
done

pull_source "http://buildroot.uclibc.org/downloads/buildroot-${BUILDROOT_VERSION}.tar.gz" "."
verify_action
pushd buildroot-${BUILDROOT_VERSION}
install_patch "../patches" "all"
install_patch "../patches" "$1"
if [ "$1" == "rbp1" ] || [ "$1" == "rbp2" ] || [ "$1" == "rbp4" ]
then
	install_patch "../patches" "rbp"
	sed s/rpi-firmware/rpi-firmware-osmc/ -i package/Config.in # Use our own firmware package
	echo "dwc_otg.fiq_fix_enable=1 sdhci-bcm2708.sync_after_dma=0 dwc_otg.lpm_enable=0 console=tty1 root=/dev/ram0 quiet init=/init loglevel=2 osmcdev=${1}" > package/rpi-firmware-osmc/cmdline.txt
fi
make osmc_defconfig
make
if [ $? != 0 ]; then echo "Build failed" && exit 1; fi
popd
pushd buildroot-${BUILDROOT_VERSION}/output/images
if [ -f ../../../filesystem.tar.xz ]
then
    echo -e "Using local filesystem"
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
if [ ! -f ../../../filesystem.tar.xz ]; then echo -e "No filesystem available for target" && exit 1; fi
echo -e "Building disk image"
if [ "$1" == "rbp1" ] || [ "$1" == "rbp2" ] || [ "$1" == "rbp4" ] || [ "$1" == "vero2" ] || [ "$1" == "vero3" ]
then
        size=320
        date=$(date +%Y%m%d)
	dd if=/dev/zero of=OSMC_TGT_${1}_${date}.img bs=1M count=${size} conv=sparse
	parted -s OSMC_TGT_${1}_${date}.img mklabel msdos
	parted -s OSMC_TGT_${1}_${date}.img mkpart primary fat32 4Mib 100%
	kpartx -s -a OSMC_TGT_${1}_${date}.img
	/sbin/partprobe
	mkfs.vfat -F32 /dev/mapper/loop0p1
	mount /dev/mapper/loop0p1 /mnt
fi
if [ "$1" == "rbp1" ] || [ "$1" == "rbp2" ] || [ "$1" == "rbp4" ]
then
	echo -e "Installing Pi files"
	mv zImage /mnt/kernel.img
	mv INSTALLER/* /mnt
	mv *.dtb /mnt
	mv overlays /mnt
fi
if [ "$1" == "vero2" ]
then
	echo -e "Installing Vero 2 files"
	../../output/build/linux-master/scripts/mkbootimg --kernel uImage --base 0x0 --kernel_offset 0x1080000 --ramdisk rootfs.cpio.gz --second ../build/linux-master/arch/arm/boot/dts/amlogic/meson8b_vero2.dtb --output /mnt/kernel.img

fi
if [ "$1" == "vero3" ]
then
	echo -e "Installing Vero 3 files"
	../.././output/build/linux-osmc-openlinux-4.9/scripts/multidtb/multidtb -o multi.dtb --dtc-path $(pwd)/../../output/build/linux-osmc-openlinux-4.9/scripts/dtc/ $(pwd)/../../output/build/linux-osmc-openlinux-4.9/arch/arm64/boot/dts/amlogic --verbose --page-size 2048
        ../../output/build/linux-osmc-openlinux-4.9/scripts/mkbootimg --kernel Image.gz --base 0x0 --kernel_offset 0x1080000 --ramdisk rootfs.cpio.gz --second multi.dtb --output /mnt/kernel.img
	cp multi.dtb /mnt/dtb.img
fi
echo -e "Installing filesystem"
mv $(pwd)/../../../filesystem.tar.xz /mnt/
umount /mnt
sync
kpartx -d OSMC_TGT_${1}_${date}.img
echo -e "Compressing image"
gzip OSMC_TGT_${1}_${date}.img
md5sum OSMC_TGT_${1}_${date}.img.gz > OSMC_TGT_${1}_${date}.md5
popd
echo -e "Build completed"
