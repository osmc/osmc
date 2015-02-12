# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../../scripts/common.sh

SYSTEM_PKGS="base-files-osmc perftune-osmc sysctl-osmc ftr-osmc diskmount-osmc apt-utils ssh-app-osmc sudo module-init-tools network-osmc locales dialog nano unzip less"
CHROOT_PKGS="${SYSTEM_PKGS}"

function setup_osmc_user()
{
	# Sets user and password to 'osmc'
	chroot ${1} useradd -p \$1\$P.ZH6EFu\$L08/1ZYI6FdHu3aw0us.u0 osmc -k /etc/skel -d /home/osmc -m -s /bin/bash
	# Locks root
	chroot ${1} passwd -l root
	# Makes 'osmc' username and password never expire
	chroot ${1} chage -I -1 -m 0 -M 99999 -E -1 osmc
	# Adds 'osmc' to sudoers with no password prompt
	echo "osmc     ALL= NOPASSWD: ALL" >${1}/etc/sudoers.d/osmc-no-sudo-password
	echo "Defaults        !secure_path" >${1}/etc/sudoers.d/osmc-no-secure-path
	chmod 0440 ${1}/etc/sudoers.d/osmc
	# Groups for permissions
	chroot ${1} usermod -G disk,cdrom,lp,dialout,video,audio,adm osmc
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

function create_fs_tarball()
{
	echo -e "Creating filesystem tarball"
	pushd ${1}
	tar -cf - * | xz -9 -c - > ../${2}-$(date +%Y%m%d).tar.xz 
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

function create_base_fstab()
{
	>${1}/etc/fstab
}

function conf_tty()
{
	chroot ${1} systemctl disable getty\@tty1.service
}

export CHROOT_PKGS

export -f setup_osmc_user
export -f setup_hostname
export -f setup_hosts
export -f create_fs_tarball
export -f disable_init
export -f enable_init
export -f create_base_fstab
export -f conf_tty
