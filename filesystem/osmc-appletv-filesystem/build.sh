# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common/funcs.sh
wd=$(pwd)
filestub="osmc-appletv-filesystem"

check_platform
verify_action

update_sources
verify_action

# Install packages needed to build filesystem for building
install_package "debootstrap"
verify_action

# Configure the target directory
ARCH="i386"
DIR="$filestub/"
RLS="jessie"

#User Information
echo"
Building for Apple TV.
This will not take too long.
"

# Remove existing build
echo Now removing the current Build..
remove_existing_filesystem "{$wd}/{$DIR}"
verify_action
echo Creating the filestub dir..
mkdir -p $DIR

# Debootstrap (foreign)

fetch_filesystem "--arch=${ARCH} --foreign --variant=minbase ${RLS} ${DIR}"
verify_action

# Configure filesystem (2nd stage)
echo Configuring..
configure_filesystem "${DIR}"
verify_action

# Enable networking
enable_nw_chroot "${DIR}"
verify_action

# Set up sources.list

echo Creating the source list..
# echo "deb http://ftp.debian.org/debian jessie main contrib non-free
echo Adding Debian Source..
deb http://ftp.debian.org/debian/ jessie-updates main contrib non-free
echo Adding Source 'Security.Debian.Org'
deb http://security.debian.org/ jessie/updates main contrib non-free
echo Adding OSMC Source
deb http://apt.osmc.tv jessie main
" > ${DIR}/etc/apt/sources.list

# Performing chroot operation
echo Working on general configuration..
disable_init "${DIR}"
chroot ${DIR} mount -t proc proc /proc
add_apt_key "${DIR}" "http://apt.osmc.tv/apt.key"
verify_action
echo -e "Updating sources"
chroot ${DIR} apt-get update
verify_action
echo -e "Installing core packages"
# We have to set up userland first for kernel postinst rules
chroot ${DIR} apt-get -y install --no-install-recommends atv-userland-osmc
verify_action
chroot ${DIR} apt-get -y install --no-install-recommends atv-device-osmc
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

# Perform filesystem cleanup
echo Just Cleaning Up..
chroot ${DIR} umount /proc
enable_init "${DIR}"
cleanup_filesystem "${DIR}"

# Create filesystem tarball
echo Making Tarball..
create_fs_tarball "${DIR}" "${filestub}"
verify_action

read -p "Done! Press any key to continue!"
