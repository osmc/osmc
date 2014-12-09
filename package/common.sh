# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../../scripts/common.sh

function fix_arch_ctl()
{
	sed '/Architecture/d' -i $1
	test $(arch)x == i686x && echo "Architecture: i386" >> $1
	test $(arch)x == armv7lx && echo "Architecture: armhf" >> $1
	test $(arch)x == x86_64x && echo "Architecture: amd64" >> $1
	sed '$!N; /^\(.*\)\n\1$/!P; D' -i $1
}

function strip_files()
{
	echo -e "Stripping binaries"
	strip "${1}/usr/lib/*.so.*" > /dev/null 2>&1
	strip "${1}/usr/lib/*.a" > /dev/null 2>&1
	strip "${1}/usr/bin/*" >/dev/null 2>&1
}

function strip_libs()
{
	echo -e "Stripping libaries"
	strip "*.so.*" > /dev/null 2>&1
	strip "*.a" > /dev/null 2>&1
}

function configure_build_env()
{
	if [ ! -f "${1}/etc/resolv.conf" ]
	then
		if [ -f "/etc/resolv.conf" ]
		then
		echo -e "Installing /etc/resolv.conf"
		cp /etc/resolv.conf ${1}/etc/resolv.conf
		fi
	fi
	if [ ! -f "${1}/etc/network/interfaces" ]
	then
		if [ -f "/etc/network/interfaces" ]
		then
		echo -e "Installing /etc/network/interfaces"
		cp /etc/network/interfaces ${1}/etc/network/interfaces
		fi
	fi
	HOSTNAME=$(cat /etc/hostname)
	echo "127.0.0.1 $HOSTNAME" > ${1}/etc/hosts
}

function build_in_env()
{
	# Don't get stuck in an endless loop
	ischroot
	if [ $? == 2 ]; then return 0; fi
	TCDIR="/opt/osmc-tc/$1-toolchain-osmc"
	handle_dep "$1-toolchain-osmc"
	if [ $? != 0 ]; then echo -e "Can't get upstream toolchain. Is apt.osmc.tv in your sources.list?" && exit 1; fi
	configure_build_env "$TCDIR"
	mount -t proc proc "$TCDIR"/proc >/dev/null 2>&1
	umount ${TCDIR}/mnt >/dev/null 2>&1 # May be dirty
	mount --bind "$2/../../" "$TCDIR"/mnt
	chroot $TCDIR /usr/bin/make $1 -C /mnt/package/$3
	return 1
}

function teardown_env()
{
	TCDIR="/opt/osmc-tc/$1-toolchain-osmc"
	umount ${TCDIR}/mnt >/dev/null 2>&1
	return 0 # Fix Make error 32
}

function handle_dep()
{
	# Used by packages that need other packages to be built first
	# Check dpkg -l for the existence of the package, try install, otherwise bail. 
	if ! dpkg -s ${1} >/dev/null 2>&1
	then
		echo -e "Package ${1} is not found on the system, checking APT"
		if ! apt-cache search ${1} > /dev/null 2>&1
		then
			echo -e "Can't find the package in APT repo. It needs to be built first or you need to wait for upstream to add it"
			return 1
		else
			echo -e "Found in APT and will install"
			install_package ${1}
		fi
	else
		echo -e "Package ${1} is already installed in the environment"
		return 0
	fi
}

export -f fix_arch_ctl
export -f strip_files
export -f strip_libs
export -f build_in_env
export -f teardown_env
export -f handle_dep
