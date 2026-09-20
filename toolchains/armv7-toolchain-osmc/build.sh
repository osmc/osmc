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
echo "deb https://deb.debian.org/debian $RLS main contrib non-free
deb https://deb.debian.org/debian/ $RLS-updates main contrib non-free
# Debian 11 (bullseye) is EOL as of 2026-08-31 and the pool behind its security suite
# has been reaped. Debian re-signed the index on 2026-09-12 with no expiry, so
# apt-get update succeeds, but most packages that index advertises now return 404.
# OSMC mirrors the final published state of the suite, so build against that rather
# than against a bullseye main that is behind what devices actually run.
# OSMC remove on Trixie release
deb https://apt.osmc.tv $RLS-security main
" > ${DIR}/etc/apt/sources.list

# Debian minbase does not ship ca-certificates yet. Work around this
inject_tls_patch ${DIR}
verify_action

# Performing chroot operation
chroot ${DIR} mount -t proc proc /proc
add_apt_key_gpg "${DIR}" "https://apt.osmc.tv/osmc_repository.gpg" "osmc_repository.gpg"
echo -e "Updating sources"
chroot ${DIR} apt-get update
verify_action
echo -e "Installing packages"
chroot ${DIR} apt-get -y install --no-install-recommends $CHROOT_PKGS
verify_action
echo -e "Adding OSMC repository"
echo "deb https://apt.osmc.tv $RLS-devel main" >> ${DIR}/etc/apt/sources.list
echo -e "Configuring ccache"
configure_ccache "${DIR}"
verify_action
echo -e "Configuring uname"
install_archlib ${DIR} "armv7l"
verify_action

# Perform filesystem cleanup
cleanup_filesystem "${DIR}"

# TLS cleanup
remove_tls_patch ${DIR}
verify_action

# Remove QEMU binary
chroot ${DIR} umount /proc
remove_emulate_arm "${DIR}" "32"

# Build Debian package
echo "Building Debian package"
build_deb_package "${wd}" "${tcstub}"
verify_action

echo -e "Build successful"
