#!/bin/bash

. ../common.sh
VERSION="0b18659f99b098d202742e94f918206ef469e713"
pull_source "https://github.com/osmc/aml-vnc-server/archive/${VERSION}.tar.gz" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "aml-vnc-app-osmc"
build_return=$?
if [ $build_return == 99 ]
then
	echo -e "Building VNC Server for Vero"
	out=$(pwd)/files
	make clean
	sed '/Package/d' -i files/DEBIAN/control
	echo "Package: ${1}-aml-vnc-app-osmc" >> files/DEBIAN/control
	update_sources
	handle_dep libvncserver-dev
	handle_dep libpng-dev
	handle_dep libssl-dev
	pushd src/aml-vnc-server-${VERSION}
	make
	mkdir -p ${out}/usr/bin
	cp -ar aml-vnc ${out}/usr/bin
	strip_files "${out}"
	popd
	fix_arch_ctl "files/DEBIAN/control"
	dpkg_build files/ ${1}-aml-vnc-app-osmc.deb
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
