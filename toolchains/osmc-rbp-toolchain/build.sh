# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common/funcs.sh
wd=$(pwd)
tcstub="osmc-rbp-toolchain"

check_platform
verify_action

update_sources
verify_action

# Install packages needed to build filesystem for building

install_package "debootstrap"
verify_action
install_package "dh-make"
verify_action
install_package "devscripts"
verify_action
install_package "qemu binfmt-support qemu-user-static"

# Configure the target directory

ARCH="armhf"
DIR="opt/osmc-tc/${tcstub}"
RLS="jessie"
URL="http://archive.raspbian.org/raspbian"

# Remove existing build

remove_existing_filesystem "{$wd}/{$DIR}"
verify_action
mkdir -p $DIR

# Debootstrap (foreign)

fetch_filesystem "--no-check-gpg --arch=${ARCH} --foreign ${RLS} ${DIR} ${URL}"
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
chroot ${DIR} mount -t proc proc /proc
LOCAL_CHROOT_PKGS="rpiuserland-dev"
add_apt_key "${DIR}" "http://apt.osmc.tv/apt.key"
add_apt_key "${DIR}" "http://archive.raspbian.org/raspbian.public.key"
verify_action
echo -e "Updating sources"
chroot ${DIR} apt-get update
verify_action
if [ ! -z $DISABLE_LOCAL_BUILDS ]
then
echo -e "Installing default chroot packages"
chroot ${DIR} apt-get -y install --no-install-recommends $CHROOT_PKGS $XBMC_MAN_PKGS_RBP
verify_action
else
echo -e "Installing default chroot packages without downstream"
chroot ${DIR} apt-get -y install --no-install-recommends $CHROOT_PKGS
verify_action
fi
if [ ! -z $DISABLE_LOCAL_BUILDS ]
then
echo -e "Installing target specific packages"
chroot ${DIR} apt-get -y install --no-install-recommends $LOCAL_CHROOT_PKGS
verify_action
else
echo -e "Told not to install target specific packages"
fi
echo -e "Configuring ccache"
configure_ccache "${DIR}"
verify_action
chroot ${DIR} umount /proc

# Remove QEMU binary
remove_emulate_arm "${DIR}"

# Perform filesystem cleanup

cleanup_filesystem "${DIR}"

# Build Debian package

echo "Building Debian package"
clean_debian_prep "${wd}" "${tcstub}"
build_deb_package "${wd}" "${tcstub}"
verify_action

# Reclean

clean_debian_prep "${wd}" "${tcstub}"
echo -e "Build successful"
