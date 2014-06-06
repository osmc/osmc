# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../../scripts/common.sh

function update_sources()
{
	echo -e "Updating sources"
	apt-get update > /dev/null 2>&1
	if [ $? != 0 ]; then echo -e "Failed to update sources" && return 1; else echo -e "Sources updated successfully" && return 0; fi
}

function install_package()

{
	echo -e "Installing package ${1}..."
	# Check if our package is installed
	dpkg --status $1 > /dev/null 2>&1
	if [ $? == 0 ]
	then
	echo -e "Package already installed."
	else
		apt-get -y install $1 > /dev/null 2>&1
		if [ $? != 0 ]; then echo -e "Failed to install" && return 1; else echo -e "Package installed successfully" && return 0; fi
	fi
}

function fetch_filesystem()
{
	echo -e "Fetching base filesystem for building target\nPlease be patient"
	debootstrap $1
	if [ $? == 0 ]
	then
	echo -e "Filesystem base install successful"
	return 0
	else
	echo -e "Filesystem base install failed"
	return 1
	fi
}

function configure_filesystem()
{
	echo -e "Configuring filesystem\nPlease be patient"
	chroot $1 /debootstrap/debootstrap --second-stage
	if [ $? == 0 ]
	then
	echo -e "Filesystem configured successfully"
	return 0
	else
	echo -e "Filesystem configuration failed"
	fi
}

function cleanup_filesystem()
{
	echo -e "Cleaning up filesystem"
	rm -f ${1}/etc/resolv.conf
	rm -f ${1}/etc/network/interfaces
	rm -rf ${1}/usr/share/doc/*
	rm -rf ${1}/usr/share/man/* 
	rm -rf ${1}/var/cache/apt/archives/*
}

function remove_existing_filesystem()
{
	if [ -f "$1" ]; then echo -e "Removing old filesystem" && rm -rf "$1"; fi
}

function clean_debian_prep()
{
	rm -rf $1/output
}

function build_deb_package()
{
	mkdir -p $1/output
	cp -ar ${1}/DEBIAN ${1}/output
	mv ${1}/opt ${1}/output
	dpkg -b ${1}/output ${2}.deb
}

COMPILER_PKGS="build-essential git subversion wget nano kernel-package osmc-appletv-darwinx"
FS_PKGS="parted kpartx"
REMOTE_PKGS="libusb-dev"
XBMC_PKGS=""
BUILDROOT_PKGS="rsync texinfo libncurses-dev whois unzip"

CHROOT_PKGS="${COMPILER_PKGS} ${FS_PKGS} ${XBMC_PKGS} ${BUILDROOT_PKGS}"
export CHROOT_PKGS

export -f update_sources
export -f install_package
export -f fetch_filesystem
export -f cleanup_filesystem
export -f remove_existing_filesystem
export -f clean_debian_prep
export -f build_deb_package
