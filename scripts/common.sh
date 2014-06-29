# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

function check_platform()
{
platform=$(lsb_release -a | grep Debian)
echo $platform | grep -q wheezy
if [ $? != 0 ]; then echo -e "We are not running Debian Wheezy" && return 1; else return 0; fi
}

function verify_action()
{
	if [ $? != 0 ]; then echo -e "Exiting build" && exit 1; fi
}

function enable_nw_chroot()
{
	echo -e "Enabling networking"
	cp /etc/resolv.conf $1/etc/
	if [ $? != 0 ]; then echo -e "Can't copy networking file" && return 1; fi
	cp /etc/network/interfaces $1/etc/network
	if [ $? != 0 ]; then echo -e "Can't copy networking file" && return 1; fi
}

function add_apt_key()
{
	echo -e "Adding apt key"
	chroot ${1} wget ${2} -O /tmp/key
	chroot ${1} apt-key add /tmp/key
	rm ${1}/tmp/key > /dev/null 2>&1
}

function emulate_arm()
{
	echo Copying ARM QEMU binary
	cp /usr/bin/qemu-arm-static ${1}/usr/bin/qemu-arm-static
}

function remove_emulate_arm()
{
	echo Removing ARM QEMU binary
	rm ${1}/usr/bin/qemu-arm-static
}

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

export -f check_platform
export -f verify_action
export -f enable_nw_chroot
export -f add_apt_key
export -f emulate_arm
export -f remove_emulate_arm
export -f update_sources
export -f install_package
export -f fetch_filesystem
export -f cleanup_filesystem
export -f remove_existing_filesystem
