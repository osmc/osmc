# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../../scripts/common.sh
. ../../package/common.sh

function build_deb_package()
{
	mkdir -p ${1}/output
	cp -ar ${1}/DEBIAN ${1}/output
	mv ${1}/opt ${1}/output
	# Mark our FS
	target=$(echo $1 | rev | cut -d / -f 1 | rev | cut -d - -f 1)
	echo ${2} >${1}/output/opt/osmc-tc/${target}-toolchain-osmc/tcver.${target}
	dpkg_build ${1}/output ${2}.deb
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

CHROOT_PKGS="build-essential nano sudo"
export CHROOT_PKGS

export -f build_deb_package
export -f patchfs
export -f configure_ccache
