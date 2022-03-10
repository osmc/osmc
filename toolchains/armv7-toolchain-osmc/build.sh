# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common/funcs.sh
wd=$(pwd)
tcstub="armv7-toolchain-osmc"

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
RLS="bullseye"
URL=""

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
configure_build_env_nw "${DIR}"
verify_action

# Set up sources.list
echo "deb http://deb.debian.org/debian $RLS main contrib non-free
deb http://deb.debian.org/debian/ $RLS-updates main contrib non-free
deb http://security.debian.org/ $RLS-security main contrib non-free
" > ${DIR}/etc/apt/sources.list

# Performing chroot operation
chroot ${DIR} mount -t proc proc /proc
add_apt_key_gpg "${DIR}" "http://apt.osmc.tv/osmc_repository.gpg" "osmc_repository.gpg"
echo -e "Updating sources"
chroot ${DIR} apt-get update
verify_action
echo -e "Installing packages"
chroot ${DIR} apt-get -y install --no-install-recommends $CHROOT_PKGS
verify_action
echo -e "Adding OSMC repository"
echo "deb http://apt.osmc.tv $RLS-devel main" >> ${DIR}/etc/apt/sources.list
echo -e "Configuring ccache"
configure_ccache "${DIR}"
verify_action
echo -e "Configuring uname"
install_archlib ${DIR} "armv7l"
verify_action

# Perform filesystem cleanup
cleanup_filesystem "${DIR}"

# Remove QEMU binary
chroot ${DIR} umount /proc
remove_emulate_arm "${DIR}" "32"

# Build Debian package
echo "Building Debian package"
build_deb_package "${wd}" "${tcstub}"
verify_action

echo -e "Build successful"
