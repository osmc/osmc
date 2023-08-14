# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common/funcs.sh
wd=$(pwd)
filestub="osmc-vero3-filesystem"

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
RLS="bullseye"

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

deb https://security.debian.org/ $RLS-security main contrib non-free

deb https://apt.osmc.tv $RLS main
" > ${DIR}/etc/apt/sources.list

# Debian minbase does not ship ca-certificates yet. Work around this
inject_tls_patch ${DIR}
verify_action

# Performing chroot operation
disable_init "${DIR}"
chroot ${DIR} mount -t proc proc /proc
add_apt_key_gpg "${DIR}" "http://apt.osmc.tv/osmc_repository.gpg" "osmc_repository.gpg"
echo -e "Enabling support for Aarch64"
chroot ${DIR} dpkg --add-architecture arm64
verify_action
echo -e "Updating sources"
chroot ${DIR} apt-get update
verify_action
echo -e "Installing core packages"
# We have to set up userland first for kernel postinst rules
chroot ${DIR} apt-get -y install --no-install-recommends vero3-userland-osmc
verify_action
chroot ${DIR} apt-get -y install --no-install-recommends vero3-device-osmc
verify_action
# We have SSH separate so we can remove it later via App Store
chroot ${DIR} apt-get -y install --no-install-recommends ssh-app-osmc
verify_action
# Ensure we have usr directory symlinks even if we use old debootstrap
chroot ${DIR} apt-get -y install --no-install-recommends usrmerge
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
echo -e "       * Configuring rc.local"
create_rc_local ${DIR}
verify_action
echo -e "       * Setting iptables to legacy"
set_iptables_to_legacy ${DIR}
verify_action
echo -e "       * Adding system release information"
add_rls_info ${DIR}
verify_action
echo -e "       * Disabling persistent journalling"
disable_persistent_journal ${DIR}
verify_action

# Perform filesystem cleanup
enable_init "${DIR}"
cleanup_filesystem "${DIR}"

# TLS cleanup
remove_tls_patch ${DIR}
verify_action

# Remove QEMU binary
chroot ${DIR} umount /proc
remove_emulate_arm "${DIR}" "32"

# Create filesystem tarball
create_fs_tarball "${DIR}" "${filestub}"
verify_action

# Move log
mv build.log ${filestub}-$(date +%Y%m%d).log
verify_action

echo -e "Build successful"
