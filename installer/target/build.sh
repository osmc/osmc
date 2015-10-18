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
hfsprogs"
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
if [ "$1" == "rbp1" ] || [ "$1" == "rbp2" ]
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
echo -e "Downloading latest filesystem"
date=$(date +%Y%m%d)
count=150
while [ $count -gt 0 ]; do wget --spider -q ${DOWNLOAD_URL}/filesystems/osmc-${1}-filesystem-${date}.tar.xz
       if [ "$?" -eq 0 ]; then
			wget ${DOWNLOAD_URL}/filesystems/osmc-${1}-filesystem-${date}.tar.xz -O filesystem.tar.xz
            break
       fi
       date=$(date +%Y%m%d --date "yesterday $date")
       let count=count-1
done
if [ ! -f filesystem.tar.xz ]; then echo -e "No filesystem available for target" && exit 1; fi
echo -e "Building disk image"
if [ "$1" == "rbp1" ] || [ "$1" == "rbp2" ] || [ "$1" == "vero1" ] || [ "$1" == "appletv" ]; then size=256; fi
date=$(date +%Y%m%d)
if [ "$1" == "rbp1" ] || [ "$1" == "rbp2" ] || [ "$1" == "vero1" ]
then
	dd if=/dev/zero of=OSMC_TGT_${1}_${date}.img bs=1M count=${size}
	parted -s OSMC_TGT_${1}_${date}.img mklabel msdos
	parted -s OSMC_TGT_${1}_${date}.img mkpart primary fat32 1M 256M
	kpartx -a OSMC_TGT_${1}_${date}.img
	mkfs.vfat -F32 /dev/mapper/loop0p1
	mount /dev/mapper/loop0p1 /mnt
fi
if [ "$1" == "appletv" ]
then
	dd if=/dev/zero of=OSMC_TGT_${1}_${date}.img bs=1M count=${size}
	parted -s OSMC_TGT_${1}_${date}.img mklabel gpt
	parted -s OSMC_TGT_${1}_${date}.img mkpart primary hfs+ 40s 256M
	parted -s OSMC_TGT_${1}_${date}.img set 1 atvrecv on
	kpartx -a OSMC_TGT_${1}_${date}.img
	mkfs.hfsplus /dev/mapper/loop0p1
	mount /dev/mapper/loop0p1 /mnt
fi
if [ "$1" == "rbp1" ] || [ "$1" == "rbp2" ]
then
	echo -e "Installing Pi files"
	mv zImage /mnt/kernel.img
	mv INSTALLER/* /mnt
fi
if [ "$1" == "vero1" ]
then
	echo -e "Installing Vero files"
	mv zImage /mnt
	mv *.dtb /mnt
	echo "mmcargs=setenv bootargs console=tty1 root=/dev/ram0 quiet init=/init loglevel=2 osmcdev=vero1 video=mxcfb0:dev=hdmi,1920x1080M@60,if=RGB24,bpp=32" > /mnt/uEnv.txt
fi
if [ "$1" == "appletv" ]
then
	echo -e "Installing AppleTV files"
	mv com.apple.Boot.plist /mnt
	sed -e "s:BOOTFLAGS:console=tty1 root=/dev/ram0 quiet init=/init loglevel=2 osmcdev=atv video=vesafb intel_idle.max_cstate=1 processor.max_cstate=2 nohpet:" -i /mnt/com.apple.Boot.plist
	mv BootLogo.png /mnt
	mv boot.efi /mnt
	mv System /mnt
	echo -e "Building mach_kernel" # Had to be done after kernel image was built
	mv bzImage ../build/atv-bootloader-master/vmlinuz
	pushd ../build/atv-bootloader-master
	make
	popd
	mv ../build/atv-bootloader-master/mach_kernel /mnt
fi
echo -e "Installing filesystem"
mv filesystem.tar.xz /mnt/
umount /mnt
sync
kpartx -d OSMC_TGT_${1}_${date}.img
if [ "$1" == "vero1" ]
then
	echo -e "Flashing bootloader"
	dd conv=notrunc if=SPL of=OSMC_TGT_${1}_${date}.img bs=1K seek=1
	dd conv=notrunc if=u-boot.img of=OSMC_TGT_${1}_${date}.img bs=1K seek=42
fi
echo -e "Compressing image"
gzip OSMC_TGT_${1}_${date}.img
md5sum OSMC_TGT_${1}_${date}.img.gz > OSMC_TGT_${1}_${date}.md5
popd
echo -e "Build completed"
