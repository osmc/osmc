# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../../scripts/common.sh

function setup_osmc_user()
{
	# Sets user and password to 'osmc'
	chroot ${1} useradd -p \$y\$j9T\$YO0bsdRQoKHvOxZPFBn5q/\$D5DzjNAvfOy.ftwO5IE2FkbrZNsC2DtiNZO9Q5.9UF1 osmc -k /etc/skel -d /home/osmc -m -s /bin/bash
	# Locks root
	chroot ${1} passwd -l root
	# Makes 'osmc' username and password never expire
	chroot ${1} chage -I -1 -m 0 -M 99999 -E -1 osmc
	# Adds 'osmc' to sudoers with no password prompt
	mkdir -p ${1}/etc/sudoers.d
	echo "osmc     ALL= NOPASSWD: ALL" >${1}/etc/sudoers.d/osmc-no-sudo-password
	echo "Defaults        !secure_path" >${1}/etc/sudoers.d/osmc-no-secure-path
	chmod 0440 ${1}/etc/sudoers.d/osmc-no-sudo-password
	chmod 0440 ${1}/etc/sudoers.d/osmc-no-secure-path
	# Groups for permissions
	chroot ${1} usermod -G disk,cdrom,lp,dialout,video,audio,adm osmc
	# Default storage directories
	directories=( "Pictures" "Music" "Movies" "TV Shows" )
	for dir in "${directories[@]}"
	do
	mkdir -p "${1}/home/osmc/${dir}"
	done
	chroot ${1} chown -R osmc:osmc /home/osmc/
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
	echo $(md5sum ../${2}-$(date +%Y%m%d).tar.xz | cut -f 1 -d ' ') filesystem.tar.xz > ../${2}-$(date +%Y%m%d).md5
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

function setup_busybox_links()
{
	if [ -f ${1}/bin/busybox ]
	then
		chroot ${1} ln -s /bin/busybox /bin/vi
		chroot ${1} ln -s /bin/busybox /bin/ping
		chroot ${1} ln -s /bin/busybox /bin/unzip
		chroot ${1} ln -s /bin/busybox /bin/nc
		chroot ${1} ln -s /bin/busybox /bin/traceroute
		chroot ${1} chmod +s /bin/busybox
	fi
}

function enable_legacy_elf()
{
	chroot ${1} ln -s /lib/arm-linux-gnueabihf/ld-linux.so.3 /lib/ld-linux.so.3
}

function create_rc_local()
{
	cat <<EOF >${1}/etc/rc.local
#!/bin/sh -e
#
# rc.local
#
# This script is executed at the end of each multiuser runlevel.
# Make sure that the script will "exit 0" on success or any other
# value on error.
#
# In order to enable or disable this script just change the execution
# bits.
#
# By default this script does nothing.

exit 0
EOF

chmod +x ${1}/etc/rc.local

}

function set_iptables_to_legacy()
{
	chroot ${1} /usr/bin/update-alternatives --set iptables /usr/sbin/iptables-legacy
	chroot ${1} /usr/bin/update-alternatives --set ip6tables /usr/sbin/ip6tables-legacy
}

function add_rls_info()
{
debootstrap_version=$(debootstrap --version | awk {'print $2'})
build_date=$(date)
hostname=$(hostname)
echo -e "OSMC filesystem assembled on ${hostname} at ${build_date} using Debootstrap version ${debootstrap_version}" > ${1}/etc/osmc_build_info
}

function disable_persistent_journal()
{
rm -rf ${1}/var/log/journal
}

export -f setup_osmc_user
export -f setup_hostname
export -f setup_hosts
export -f create_fs_tarball
export -f disable_init
export -f enable_init
export -f create_base_fstab
export -f conf_tty
export -f setup_busybox_links
export -f enable_legacy_elf
export -f create_rc_local
export -f set_iptables_to_legacy
export -f add_rls_info
export -f disable_persistent_journal
