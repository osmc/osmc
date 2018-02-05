# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common/funcs.sh
wd=$(pwd)
filestub="osmc-vero2-filesystem"

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
RLS="stretch"

# Remove existing build
remove_existing_filesystem "{$wd}/{$DIR}"
verify_action
mkdir -p $DIR

# Debootstrap (foreign)

fetch_filesystem "--no-check-gpg --arch=${ARCH} --foreign --variant=minbase ${RLS} ${DIR} ${URL}"
verify_action

# Configure filesystem (2nd stage)
emulate_arm "${DIR}" "32"

configure_filesystem "${DIR}"
verify_action

# Enable networking
enable_nw_chroot "${DIR}"
verify_action

# Set up sources.list
echo "deb http://ftp.debian.org/debian $RLS main contrib non-free

deb http://ftp.debian.org/debian/ $RLS-updates main contrib non-free

deb http://security.debian.org/ $RLS/updates main contrib non-free

deb http://apt.osmc.tv $RLS main
" > ${DIR}/etc/apt/sources.list

# Performing chroot operation
disable_init "${DIR}"
chroot ${DIR} mount -t proc proc /proc
add_apt_key_gpg "${DIR}" "http://apt.osmc.tv/osmc_repository.gpg" "osmc_repository.gpg"
echo -e "Updating sources"
chroot ${DIR} apt-get update
verify_action
echo -e "Installing core packages"
# We have to set up userland first for kernel postinst rules
chroot ${DIR} apt-get -y install --no-install-recommends vero2-userland-osmc
verify_action
chroot ${DIR} apt-get -y install --no-install-recommends vero2-device-osmc
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
echo -e "       * Enabling support for legacy ELF"
enable_legacy_elf ${DIR}
verify_action
echo -e "       * Configuring rc.local"
create_rc_local ${DIR}
verify_action

# Perform filesystem cleanup
enable_init "${DIR}"
cleanup_filesystem "${DIR}"

# Remove QEMU binary
chroot ${DIR} umount /proc
remove_emulate_arm "${DIR}" "32"

# Create filesystem tarball
create_fs_tarball "${DIR}" "${filestub}"
verify_action

echo -e "Build successful"
