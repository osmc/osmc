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
	target=$(echo $t1 | cut -f 1 -d -)
	echo ${2} >${1}/output/tcver.$target
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
}

function cleanup_buildcache()
{
	echo -e "Deleting ccache data"
	rm -rf ${1}/root/.ccache
}

CHROOT_PKGS="build-essential nano sudo"
export CHROOT_PKGS

export -f build_deb_package
export -f patchfs
export -f configure_ccache
export -f cleanup_buildcache
