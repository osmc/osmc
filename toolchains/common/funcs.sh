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
	rm -rf ${1}/output
}

function build_deb_package()
{
	mkdir -p ${1}/output
	cp -ar ${1}/DEBIAN ${1}/output
	mv ${1}/opt ${1}/output
	# Mark our FS
	echo ${2} ${1}/output/tcver
	dpkg -b ${1}/output ${2}.deb
}

function patchfs()
{
	cp ${1}../../../../patches/${2} ${1}
	chroot ${1} /usr/bin/patch ${2}
}

function configure_ccache()
{
	chroot ${1} apt-get -y install --no-install-recommends ccache
	chroot ${1} /usr/bin/ccache -M 20G
	echo -e "PATH=/usr/lib/ccache:\${PATH}" >> ${1}/root/.bashrc
	cp ../common/populate-osmc-cache ${1}/usr/bin
	chmod +x ${1}/usr/bin/populate-osmc-cache
}

COMPILER_PKGS="build-essential git subversion wget nano kernel-package sudo"
FS_PKGS="parted kpartx libblkid-dev libfuse-dev libreadline-dev"
REMOTE_PKGS="libusb-dev"
XBMC_PKGS="autopoint automake bison make curl cvs default-jre fp-compiler gawk gdc gettext git-core gperf libasound2-dev libass-dev libboost-dev libboost-thread-dev libbz2-dev libcap-dev libcdio-dev libcurl3 libcurl4-gnutls-dev libdbus-1-dev libenca-dev libflac-dev libfontconfig-dev libfreetype6-dev libfribidi-dev libglew-dev libiso9660-dev libjasper-dev libjpeg-dev liblzo2-dev libmad0-dev libmicrohttpd-dev libmodplug-dev libmpeg2-4-dev libmpeg3-dev libmysqlclient-dev libnfs-dev libogg-dev libogg-dev libpcre3-dev libplist-dev libpng-dev libpulse-dev libsdl-dev libsdl-gfx1.2-dev libsdl-image1.2-dev libsdl-mixer1.2-dev libsmbclient-dev libsqlite3-dev libssh-dev libssl-dev libtiff-dev libtinyxml-dev libtool libudev-dev libusb-dev libvdpau-dev libvorbisenc2 libxml2-dev libxmu-dev libxrandr-dev libxrender-dev libxslt1-dev libxt-dev libyajl-dev mesa-utils nasm pmount python-dev python-imaging python-sqlite swig unzip yasm zip zlib1g-dev libsmbclient-dev libbluray-dev libtag1-dev libsamplerate-dev librtmp-dev libmp3lame-dev libltdl-dev libafpclient-dev cmake"
XBMC_MAN_PKGS_GENERIC="libcecdev-osmc libshairplay-dev"
XBMC_MAN_PKGS_RBP="rbp-libcecdev-osmc rbp-libshairplaydev-osmc"
BUILDROOT_PKGS="rsync texinfo libncurses-dev whois unzip"

CHROOT_PKGS="${COMPILER_PKGS} ${REMOTE_PKGS} ${FS_PKGS} ${XBMC_PKGS} ${BUILDROOT_PKGS}"
export CHROOT_PKGS
export XBMC_MAN_PKGS
export XBMC_MAN_PKGS_RBP

export -f update_sources
export -f install_package
export -f fetch_filesystem
export -f cleanup_filesystem
export -f remove_existing_filesystem
export -f clean_debian_prep
export -f build_deb_package
export -f patchfs
export -f configure_ccache
