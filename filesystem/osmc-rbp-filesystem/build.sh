# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common/funcs.sh
wd=$(pwd)
filestub="osmc-rbp-filesystem"

check_platform
verify_action

update_sources
verify_action

# Install packages needed to build filesystem for building
install_package "debootstrap"
verify_action

# Configure the target directory
ARCH="armhf"
DIR="$filestub/"
RLS="jessie"
URL="http://archive.raspbian.org/raspbian"

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
echo "deb http://archive.raspbian.org/raspbian jessie main contrib non-free
deb http://apt.osmc.tv jessie main
" > ${DIR}/etc/apt/sources.list

# Performing chroot operation
disable_init "${DIR}"
chroot ${DIR} mount -t proc proc /proc
LOCAL_CHROOT_PKGS="rbp-bootloader-osmc rbp-splash-osmc rbp-armmem-osmc rbp-userland-osmc rbp-kernel-osmc"
add_apt_key "${DIR}" "http://apt.osmc.tv/apt.key"
verify_action
echo -e "Updating sources"
chroot ${DIR} apt-get update
verify_action
echo -e "Installing core packages"
chroot ${DIR} apt-get -y install --no-install-recommends $CHROOT_PKGS
verify_action
chroot ${DIR} apt-get -y install --no-install-recommends $LOCAL_CHROOT_PKGS
verify_action
chroot ${DIR} apt-get -y install --no-install-recommends rbp-mediacenter-osmc
echo -e "Configuring environment"
echo -e "	* Adding user osmc"
setup_osmc_user ${DIR}
verify_action
echo -e "	* Configuring VCHIQ"
configure_vchiq_udev ${DIR}
echo -e "	* Setting hostname"
setup_hostname ${DIR}
verify_action
echo -e "	* Setting up hosts"
setup_hosts ${DIR}
verify_action
echo -e "	* Configuring fstab"
create_base_fstab ${DIR}
verify_action
echo -e "	* Holding back packages"
prevent_pkg_install "${DIR}" "xbmc"
verify_action
echo -e "	* Configuring TTYs"
conf_tty ${DIR}
verify_action

# Remove QEMU binary
chroot ${DIR} umount /proc
remove_emulate_arm "${DIR}"

# Perform filesystem cleanup
enable_init "${DIR}"
cleanup_filesystem "${DIR}"

# Create filesystem tarball
create_fs_tarball "${DIR}" "${filestub}"
verify_action

echo -e "Build successful"
