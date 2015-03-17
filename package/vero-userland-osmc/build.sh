# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

pull_source "http://www.freescale.com/lgfiles/NMG/MAD/YOCTO/imx-lib-3.10.17-1.0.0.tar.gz" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
rm -rf src/imx-lib*/hdmi-cec
pull_bin "http://www.freescale.com/lgfiles/NMG/MAD/YOCTO/firmware-imx-3.10.17-1.0.0.bin" "$(pwd)/src/firmware-imx.bin"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
pull_bin "http://www.freescale.com/lgfiles/NMG/MAD/YOCTO/imx-vpu-3.10.17-1.0.0.bin" "$(pwd)/src/imx-vpu.bin"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
pull_bin "http://www.freescale.com/lgfiles/NMG/MAD/YOCTO/libfslvpuwrap-1.0.46.bin" "$(pwd)/src/libfslvpuwrap.bin"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
pull_bin "http://www.freescale.com/lgfiles/NMG/MAD/YOCTO/gpu-viv-bin-mx6q-3.10.17-1.0.0-hfp.bin" "$(pwd)/src/gpu-viv.bin"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "vero-userland-osmc"
if [ $? == 0 ]
then
	echo -e "Building package vero-userland-osmc"
	out=$(pwd)/files
	make clean
	update_sources
	handle_dep "libtool-bin"
	handle_dep "autoconf"
	handle_dep "automake"
	pushd src
	install_patch "../patches" "all"
	rm -rf /headers > /dev/null 2>&1
	cp -ar headers /
	pushd imx-lib*
	sed -i */Makefile -e s/-O2/-O3/
	$BUILD PLATFORM=IMX6Q C_INCLUDE_PATH=/headers/include/ all
	make install PLATFORM=IMX6Q DEST_DIR=${out}
	if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
	popd
	sh firmware-imx.bin --auto-accept
	pushd firmware-imx*
	mkdir -p ${out}/lib
	cp -ar firmware ${out}/lib
	popd
	sh imx-vpu.bin --auto-accept
	pushd imx-vpu*
	sed -i */Makefile -e s/-O2/-O3/
	$BUILD PLATFORM=IMX6Q
	if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
	make install PLATFORM=IMX6Q DEST_DIR=${out} # of course, Freescale like to hop between DESTDIR and DEST_DIR
	popd
	sh libfslvpuwrap.bin --auto-accept
	pushd libfslvpuwrap*
	CFLAGS="-I../../files/usr/include -L../../files/usr/lib" ./autogen.sh --prefix=/usr
	$BUILD all
	if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
	make install DESTDIR=${out}
	popd
	sh gpu-viv.bin --auto-accept
	# Remove samples
	rm -rf gpu-viv-bin-mx6q*/opt
	# Remove conflicting libraries
	pushd gpu-viv-bin-mx6q*
	pushd usr/lib
	rm libGAL.so libVIVANTE.so libEGL.so *-wl.so* *wayland* *-dfb.so* *-x11.so*
	ln -s libEGL-fb.so libEGL.so
	ln -s libGAL-fb.so libGAL.so
	ln -s libVIVANTE-fb.so libVIVANTE.so
	popd
	cp -ar usr ${out}
	popd
	if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
	strip_libs
	popd
	mkdir -p files-dev/usr
	mv files/usr/include files-dev/usr/
	rm -rf ${out}/usr/share/imx-mm/video-codec
	rm -rf ${out}/usr/share/doc/
	dpkg -b files/ vero-userland-osmc.deb
	dpkg -b files-dev vero-userland-dev-osmc.deb
fi
teardown_env "${1}"
