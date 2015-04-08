# (c) 2014-2015 Sam Nazarko
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
	strip "${1}/usr/sbin/*" >/dev/null 2>&1
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
	export LANG=C
	# Don't get stuck in an endless loop
	mount -t proc proc /proc >/dev/null 2>&1
	ischroot
	chrootval=$?
	if [ $chrootval == 2 ] || [ $chrootval == 0 ]; then return 99; fi
	umount /proc >/dev/null 2>&1
	update_sources
	DEP=${1}
	test $DEP == rbp2 && DEP="armv7"
	test $DEP == imx6 && DEP="armv7"
	test $DEP == vero && DEP="armv7"
	test $DEP == vero1 && DEP="armv7"
	test $DEP == rbp1 && DEP="armv6l"
	test $DEP == atv && DEP="i386"
	TCDIR="/opt/osmc-tc/$DEP-toolchain-osmc"
	handle_dep "$DEP-toolchain-osmc"
	if [ $? != 0 ]; then echo -e "Can't get upstream toolchain. Is apt.osmc.tv in your sources.list?" && exit 1; fi
	configure_build_env "$TCDIR"
	umount ${TCDIR}/mnt >/dev/null 2>&1 # May be dirty
	mount --bind "$2/../../" "$TCDIR"/mnt
	chroot $TCDIR /usr/bin/make $1 -C /mnt/package/$3
	return=$?
	if [ $return == 99 ]; then return 1; else return $return; fi
}

function teardown_env()
{
	TCDIR="/opt/osmc-tc/$1-toolchain-osmc"
	umount ${TCDIR}/mnt >/dev/null 2>&1
}

function handle_dep()
{
	# Used by packages that need other packages to be built first
	# Check dpkg -l for the existence of the package, try install, otherwise bail. 
	if ! dpkg -s ${1} >/dev/null 2>&1
	then
		echo -e "Package ${1} is not found on the system, checking APT"
		# apt-cache search always returns 0. Ugh. 
		packages=$(apt-cache search ${1} | wc -l)
		if [ "$packages" -eq 0 ]
		then
			echo -e "Can't find the package in APT repo. It needs to be built first or you need to wait for upstream to add it"
			return 1
		else
			echo -e "Found in APT and will install"
			# armv7 conflicts
			if [ "$1" == "vero-userland-dev-osmc" ]; then remove_conflicting "rbp-userland-dev-osmc"; fi
			if [ "$1" == "rbp-userland-dev-osmc" ]; then remove_conflicting "vero-userland-dev-osmc"; fi
			install_package ${1}
		fi
	else
		echo -e "Package ${1} is already installed in the environment"
		return 0
	fi
}

function publish_applications_any()
{
	# Used by applications that are architecture independent. These are usually metapackages with some configuration files shipped.
	PKG_TARGETS="rbp1 rbp2 atv vero"
	for TARGET in $PKG_TARGETS
	do
		echo -e "Publishing application for platform ${TARGET}"
		# No need to change id. Architecture is Any. 
		cp ${1}/app.json ${1}/${TARGET}-${2}.json
	done
}

function publish_applications_targeted()
{
	# Used by applications that are architecture dependent. 
	echo -e "Publishing application for platform ${TARGET}"
	# This is a tad hacky. Architecture specific, platform independent
	if [ "$2" == "armv6l" ]; then devices="rbp1"; fi
	if [ "$2" == "armv7" ]; then devices="rbp2 vero"; fi
	if [ "$2" == "i386" ]; then devices="atv"; fi
	# Architecture specific, platform specific
	if [ "$2" == "rbp1" ]; then devices="rbp1"; fi
	if [ "$2" == "rbp2" ]; then devices="rbp2"; fi
	if [ "$2" == "vero" ]; then devices="vero"; fi
	if [ "$2" == "atv1" ]; then devices="atv"; fi
	for device in $devices
	do
	    cp ${1}/app.json ${1}/${device}-${3}.json
	    sed -e s/\"id\":\ \"/\"id\":\ \"${2}-/ -i ${device}-${3}.json # set the correct package id
	done
}

function remove_conflicting()
{
	# This is not ideal...
	ischroot
	chrootval=$?
	# guard
	if [ $chrootval == 2 ] || [ $chrootval == 0 ]
	then
		dpkg --list | grep -q $1
		if [ $? == 0 ]; then apt-get remove --purge $1; fi
	fi
}

export -f fix_arch_ctl
export -f strip_files
export -f strip_libs
export -f build_in_env
export -f teardown_env
export -f handle_dep
export -f publish_applications_any
export -f publish_applications_targeted
export -f remove_conflicting
