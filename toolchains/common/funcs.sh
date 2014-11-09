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
	echo ${2} >${1}/output/tcver
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
}

function cleanup_buildcache()
{
	echo -e "Deleting ccache data"
	rm -rf ${1}/root/.ccache
}

function set_lb()
{
	[ -f /tmp/disable-lb ] && export DISABLE_LOCAL_BUILDS=1
}

function set_publish()
{
	[ -f /tmp/publish-tc ] && export ENABLE_PUBLISH_BUILDS=1
}

CHROOT_PKGS="build-essential nano sudo"
export CHROOT_PKGS
export XBMC_MAN_PKGS
export XBMC_MAN_PKGS_RBP

export -f build_deb_package
export -f patchfs
export -f configure_ccache
export -f cleanup_buildcache
export -f set_lb
