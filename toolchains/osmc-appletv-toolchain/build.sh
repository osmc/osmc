# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common/funcs.sh

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

# Configure the target directory

ARCH="i386"
DIR="tc"
RLS="jessie"

# Remove existing build

remove_existing_filesystem
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

# Performing chroot operation

#cp -ar "chroot_build.sh" "${DIR}"
#chroot "${DIR}" "/chroot_build.sh"
#verify_action

# Perform filesystem cleanup

cleanup_filesystem "${DIR}"

# Build Debian package

wd=$(pwd)
clean_debian_prep "${wd}"
echo "Building Debian package"
build_deb_package "${wd}"
verify_action

# Move package up
mv ../*.deb ${wd}

# Reclean

clean_debian_prep "${wd}"
remove_existing_filesystem

echo -e "Build successful"
