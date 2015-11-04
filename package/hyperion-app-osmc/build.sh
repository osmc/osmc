# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh
VERSION="ccb9b98f3e42785620ade9970b33d94c77b93508"
pull_source "https://github.com/tvdzwan/hyperion/archive/${VERSION}.tar.gz" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "hyperion-app-osmc"
build_return=$?
if [ $build_return == 99 ]
then
	echo -e "Building Hyperion"
	out=$(pwd)/files
	sed '/Package/d' -i files/DEBIAN/control
	rm -f files/etc/osmc/apps.d/*hyperion-app-osmc
	update_sources
	handle_dep "cmake"
	handle_dep "libqt4-dev"
	handle_dep "libusb-1.0-0-dev"
	handle_dep "python-dev"
	handle_dep "protobuf-compiler"
	if [ "$1" == "rbp1" ] || [ "$1" == "rbp2" ]
        then
                handle_dep "rbp-userland-dev-osmc"
        fi
	mkdir -p files/etc/osmc/apps.d
	echo "Package: ${1}-hyperion-app-osmc" >> files/DEBIAN/control && APP_FILE="files/etc/osmc/apps.d/${1}-hyperion-app-osmc"
    	echo -e "Hyperion\nhyperion.service" > $APP_FILE
	pushd src/hyperion*
	install_patch "../../patches" "all"
	mkdir build
	pushd build
	cmake ../
	$BUILD
	# Hyperion does not make install -- use install one day?
	cp ./bin/hyperion-remote /usr/bin/
	cp ./bin/hyperiond /usr/bin/
	if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
	strip_files "${out}"
	popd
	popd
	fix_arch_ctl "files/DEBIAN/control"
	publish_applications_targeted "$(pwd)" "$1" "hyperion-app-osmc"
	dpkg_build files/ ${1}-hyperion-app-osmc.deb
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
