# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh
VERSION="1.2.0"
pull_source "https://github.com/osmc/aml-vnc-server/archive/${VERSION}.tar.gz" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "aml-vnc-app-osmc"
build_return=$?
if [ $build_return == 99 ]
then
	echo -e "Building VNC Server for Vero"
	out=$(pwd)/files
	sed '/Package/d' -i files/DEBIAN/control
	rm -f files/etc/osmc/apps.d/*aml-vnc-app-osmc
	update_sources
	handle_dep libvncserver-dev
	handle_dep libpng-dev
	handle_dep libssl-dev
	mkdir -p files/etc/osmc/apps.d
	echo "Package: ${1}-aml-vnc-app-osmc" >> files/DEBIAN/control && APP_FILE="files/etc/osmc/apps.d/${1}-aml-vnc-app-osmc"
	echo -e "VNC Server\naml-vnc.service" > $APP_FILE
	pushd src/aml-vnc-server*
	make
	mkdir -p ${out}/usr/bin
	cp -ar aml-vnc ${out}/usr/bin
	strip_files "${out}"
	popd
	fix_arch_ctl "files/DEBIAN/control"
	publish_applications_targeted "$(pwd)" "$1" "aml-vnc-app-osmc"
	dpkg_build files/ ${1}-aml-vnc-app-osmc.deb
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
