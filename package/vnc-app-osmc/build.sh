# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh
REV="6fe81a98a9725787c27d20f2edb48aa026a8cd35"
if [ "$1" == "rbp1" ] || [ "$1" == "rbp2" ]
then
    pull_source "https://github.com/osmc/dispmanx_vnc/archive/${REV}.tar.gz" "$(pwd)/src"
fi
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "vnc-app-osmc"
build_return=$?
if [ $build_return == 99 ]
then
	echo -e "Building VNC Server"
	out=$(pwd)/files
	sed '/Package/d' -i files/DEBIAN/control
	rm -f files/etc/osmc/apps.d/*vnc-app-osmc
	update_sources
	handle_dep "libvncserver-dev"
	if [ "$1" == "rbp1" ] || [ "$1" == "rbp2" ]
	then
	    handle_dep "rbp-userland-dev-osmc"
	    handle_dep "libconfig++-dev"
	fi
	mkdir -p files/etc/osmc/apps.d
	echo "Package: ${1}-vnc-app-osmc" >> files/DEBIAN/control && APP_FILE="files/etc/osmc/apps.d/${1}-vnc-app-osmc"
    echo -e "VNC Server\nvnc.service" > $APP_FILE
    pushd src/dispmanx_vnc*
    $BUILD
    if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
    strip_files "${out}"
    mkdir -p ${out}/usr/bin
    cp -ar  dispmanx_vncserver ${out}/usr/bin/
    mkdir -p ${out}/lib/systemd/system/
    if [ "$1" == "rbp1" ] || [ "$1" == "rbp2" ]
    then
	cp -ar rbp-vnc.service ${out}/lib/systemd/system/vnc.service
    fi
    popd
    fix_arch_ctl "files/DEBIAN/control"
    publish_applications_targeted "$(pwd)" "$1" "vnc-app-osmc"
    dpkg_build files/ ${1}-vnc-app-osmc.deb
    build_return=$?
fi
teardown_env "${1}"
exit $build_return
