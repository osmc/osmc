# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../../scripts/common.sh

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
XBMC_MAN_PKGS="libcecdev-osmc libshairplaydev-osmc librtmpdev-osmc"
XBMC_MAN_PKGS_RBP="rbp-libcecdev-osmc rbp-libshairplaydev-osmc"
BUILDROOT_PKGS="rsync texinfo libncurses-dev whois unzip"

CHROOT_PKGS="${COMPILER_PKGS} ${REMOTE_PKGS} ${FS_PKGS} ${XBMC_PKGS} ${BUILDROOT_PKGS}"
export CHROOT_PKGS
export XBMC_MAN_PKGS
export XBMC_MAN_PKGS_RBP

export -f build_deb_package
export -f patchfs
export -f configure_ccache
