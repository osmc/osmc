# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common/funcs.sh
wd=$(pwd)
filestub="osmc-rbp2-filesystem"

check_platform
verify_action

update_sources
verify_action

# Install packages needed to build filesystem for building
packages="debootstrap
qemu
binfmt-support
qemu-user-static"
for package in $packages
do
	install_package $package
	verify_action
done

# Configure the target directory
ARCH="armhf"
DIR="$filestub/"
RLS="jessie"

# Remove existing build
remove_existing_filesystem "{$wd}/{$DIR}"
verify_action
mkdir -p $DIR

# Debootstrap (foreign)

fetch_filesystem "--no-check-gpg --arch=${ARCH} --foreign --variant=minbase ${RLS} ${DIR} ${URL}"
verify_action

# Configure filesystem (2nd stage)
emulate_arm "${DIR}"

configure_filesystem "${DIR}"
verify_action

# Enable networking
enable_nw_chroot "${DIR}"
verify_action

# Set up sources.list
echo "deb http://ftp.debian.org/debian jessie main contrib non-free

deb http://ftp.debian.org/debian/ jessie-updates main contrib non-free

deb http://security.debian.org/ jessie/updates main contrib non-free

deb http://staging.apt.osmc.tv jessie main
" > ${DIR}/etc/apt/sources.list

# Performing chroot operation
disable_init "${DIR}"
chroot ${DIR} mount -t proc proc /proc
add_apt_key "${DIR}" "http://apt.osmc.tv/apt.key"
verify_action
echo -e "Updating sources"
chroot ${DIR} apt-get update
verify_action
echo -e "Installing core packages"
# We have to set up userland first for kernel postinst rules
chroot ${DIR} apt-get -y install --no-install-recommends rbp-userland-osmc
verify_action
chroot ${DIR} apt-get -y install --no-install-recommends rbp2-device-osmc
verify_action
# We have SSH separate so we can remove it later via App Store
chroot ${DIR} apt-get -y install --no-install-recommends ssh-app-osmc
verify_action
echo -e "Configuring environment"
echo -e "	* Adding user osmc"
setup_osmc_user ${DIR}
verify_action
echo -e "	* Setting hostname"
setup_hostname ${DIR}
verify_action
echo -e "	* Setting up hosts"
setup_hosts ${DIR}
verify_action
echo -e "	* Configuring fstab"
create_base_fstab ${DIR}
verify_action
echo -e "	* Configuring TTYs"
conf_tty ${DIR}
verify_action
echo -e "	* Configuring BusyBox symlinks"
setup_busybox_links ${DIR}
verify_action

# Remove QEMU binary
chroot ${DIR} umount /proc
remove_emulate_arm "${DIR}"

# We do this after ARM user emulation removal, because QEMU gets broken by this
echo -e "       * Configuring optimised string.h operations"
MEM_OPTIM="/usr/lib/libarmmem.so"
echo ${MEM_OPTIM} > ${DIR}/etc/ld.so.preload

# Perform filesystem cleanup
enable_init "${DIR}"
cleanup_filesystem "${DIR}"
enable_mirrordirector "${DIR}"

# Create filesystem tarball
create_fs_tarball "${DIR}" "${filestub}"
verify_action

echo -e "Build successful"
