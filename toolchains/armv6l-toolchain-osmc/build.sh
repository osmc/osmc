# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common/funcs.sh
wd=$(pwd)
tcstub="armv6l-toolchain-osmc"

make clean

check_platform
verify_action

update_sources
verify_action

# Install packages needed to build filesystem for building
packages="debootstrap
dh-make
devscripts
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
DIR="opt/osmc-tc/${tcstub}"
RLS="jessie"
URL="http://mirrordirector.raspbian.org/raspbian"

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
echo "deb http://mirrordirector.raspbian.org/raspbian jessie main contrib non-free
deb http://staging.apt.osmc.tv jessie-devel main
" > ${DIR}/etc/apt/sources.list

# Performing chroot operation
chroot ${DIR} mount -t proc proc /proc
add_apt_key "${DIR}" "http://apt.osmc.tv/apt.key"
add_apt_key "${DIR}" "http://mirrordirector.raspbian.org/raspbian.public.key"
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

# Remove QEMU binary
chroot ${DIR} umount /proc
remove_emulate_arm "${DIR}"

# Perform filesystem cleanup
cleanup_filesystem "${DIR}"

# Build Debian package
echo "Building Debian package"
build_deb_package "${wd}" "${tcstub}"
verify_action

echo -e "Build successful"
