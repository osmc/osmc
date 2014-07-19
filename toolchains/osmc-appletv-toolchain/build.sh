# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common/funcs.sh
wd=$(pwd)
tcstub="osmc-appletv-toolchain"

check_platform
verify_action

update_sources
verify_action

set_lb

# Install packages needed to build filesystem for building
install_package "debootstrap"
verify_action
install_package "dh-make"
verify_action
install_package "devscripts"
verify_action

# Configure the target directory

ARCH="i386"
DIR="opt/osmc-tc/${tcstub}"
RLS="jessie"

# Remove existing build

remove_existing_filesystem "{$wd}/{$DIR}"
verify_action
mkdir -p $DIR

# Debootstrap (foreign)

fetch_filesystem "--arch=${ARCH} --foreign ${RLS} ${DIR}"
verify_action

# Configure filesystem (2nd stage)
configure_filesystem "${DIR}"
verify_action

# Enable networking
enable_nw_chroot "${DIR}"
verify_action

# Set up sources.list
echo "deb http://ftp.debian.org/debian jessie main contrib

deb http://ftp.debian.org/debian/ jessie-updates main contrib

deb http://security.debian.org/ jessie/updates main contrib

deb http://apt.osmc.tv jessie main
" > ${DIR}/etc/apt/sources.list

# Performing chroot operation
chroot ${DIR} mount -t proc proc /proc
LOCAL_CHROOT_PKGS="osmc-appletv-darwinx libcrystalhd-dev"
add_apt_key "${DIR}" "http://apt.osmc.tv/apt.key"
verify_action
echo -e "Updating sources"
chroot ${DIR} apt-get update
verify_action
if [ -z $DISABLE_LOCAL_BUILDS ]
then
echo -e "Installing default chroot packages"
chroot ${DIR} apt-get -y install --no-install-recommends $CHROOT_PKGS $XBMC_MAN_PKGS
verify_action
else
echo -e "Installing default chroot packages without downstream"
chroot ${DIR} apt-get -y install --no-install-recommends $CHROOT_PKGS
verify_action
fi
if [ -z $DISABLE_LOCAL_BUILDS ]
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

# Perform filesystem cleanup
chroot ${DIR} umount /proc
cleanup_filesystem "${DIR}"

# Build Debian package
echo "Building Debian package"
build_deb_package "${wd}" "${tcstub}"
verify_action

echo -e "Build successful"
