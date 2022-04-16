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
	mkdir -p ${1}/root/.ccache
}

function install_archlib()
{
	cp ../common/uname-osmc.c ${1}
	chroot ${1} gcc -DTARGET_ARCH=\"${2}\" -fPIC -c /uname-osmc.c
	chroot ${1} gcc -shared -o /usr/lib/uname-osmc.so uname-osmc.o
	echo "/usr/lib/uname-osmc.so" >> ${1}/etc/ld.so.preload
	rm -f ${1}/uname-osmc.o
	rm -f ${1}/uname-osmc.c
}

CHROOT_PKGS="build-essential nano sudo libeatmydata1 ca-certificates usrmerge"
export CHROOT_PKGS

export -f build_deb_package
export -f patchfs
export -f configure_ccache
