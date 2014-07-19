# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../../scripts/common.sh

XBMC_MAN_ATV="mediacenter-atv-osmc"
XBMC_MAN_RBP="mediacenter-rbp-osmc"
INIT_PKGS="systemd systemd-sysv"
SYSTEM_PKGS="base-files sysctl-osmc openssh-server sudo"
CHROOT_PKGS="${INIT_PKGS} ${SYSTEM_PKGS}"

function setup_osmc_user()
{
	# Sets user and password to 'osmc'
	chroot ${1} useradd -p \$1\$P.ZH6EFu\$L08/1ZYI6FdHu3aw0us.u0 osmc -k /etc/skel -d /home/osmc -m -s /bin/bash
	# Locks root
	chroot ${1} passwd -l root
	# Makes 'osmc' username and password never expire
	chroot ${1} chage -I -1 -m 0 -M 99999 -E -1 osmc
	# Adds 'osmc' to sudoers with no password prompt
	echo "osmc     ALL= NOPASSWD: ALL" >${1}/etc/sudoers.d/osmc
	chmod 0440 ${1}/etc/sudoers.d/osmc
}

function setup_hostname()
{
	echo "osmc" > ${1}/etc/hostname
}

function setup_hosts()
{
	echo "::1             osmc localhost6.localdomain6 localhost6
127.0.1.1       osmc


127.0.0.1       localhost
::1             localhost ip6-localhost ip6-loopback
fe00::0         ip6-localnet
ff00::0         ip6-mcastprefix
ff02::1         ip6-allnodes
ff02::2         ip6-allrouters">${1}/etc/hosts
}

function prevent_pkg_install()
{
	echo "Package: ${2}
Pin: release \*
Pin-Priority: -1" > ${1}/etc/apt/preferences.d/${2}
}

function configure_vchiq_udev()
{
	echo 'SUBSYSTEM=="vchiq",  GROUP="video", MODE="0660"'>${1}etc/udev/rules.d/10-permissions.rules
}

function create_fs_tarball()
{
	echo -e "Creating filesystem tarball"
	pushd ${1}
	tar -czf ../${2}.tar.gz *
	popd
	rm -rf ${1}
}

function disable_init()
{
	echo "exit 101" >${1}/usr/sbin/policy-rc.d
	chmod 0755 ${1}/usr/sbin/policy-rc.d
}

function enable_init()
{
	rm ${1}/usr/sbin/policy-rc.d
}

export CHROOT_PKGS
export XBMC_MAN_ATV
export XBMC_MAN_RBP

export -f setup_osmc_user
export -f setup_hostname
export -f setup_hosts
export -f prevent_pkg_install
export -f create_fs_tarball
export -f disable_init
export -f enable_init
