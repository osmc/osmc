# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common/funcs.sh
wd=$(pwd)
tcstub="i386-toolchain-osmc"

make clean

check_platform
verify_action

update_sources
verify_action

# Install packages needed to build filesystem for building
packages="debootstrap
dh-make
devscripts"
for package in $packages
do
	install_package $package
	verify_action
done

# Configure the target directory
ARCH="i386"
DIR="opt/osmc-tc/${tcstub}"
RLS="jessie"

# Remove existing build
remove_existing_filesystem "{$wd}/{$DIR}"
verify_action
mkdir -p $DIR

# Debootstrap (foreign)
fetch_filesystem "--arch=${ARCH} --foreign --variant=minbase ${RLS} ${DIR}"
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

deb http://staging.apt.osmc.tv jessie-devel main
" > ${DIR}/etc/apt/sources.list

# Performing chroot operation
chroot ${DIR} mount -t proc proc /proc
add_apt_key "${DIR}" "http://apt.osmc.tv/apt.key"
verify_action
echo -e "Updating sources"
chroot ${DIR} apt-get update
verify_action
echo -e "Installing packages"
chroot ${DIR} apt-get -y install --no-install-recommends $CHROOT_PKGS
verify_action
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
