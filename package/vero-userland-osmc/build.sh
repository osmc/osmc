# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

pull_source "http://www.freescale.com/lgfiles/NMG/MAD/YOCTO/imx-lib-3.10.31-1.1.0-beta.tar.gz" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
rm -rf src/imx-lib*/hdmi-cec
pull_bin "http://www.freescale.com/lgfiles/NMG/MAD/YOCTO/firmware-imx-3.10.17-1.0.0.bin" "$(pwd)/src/firmware-imx.bin"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
pull_bin "http://www.freescale.com/lgfiles/NMG/MAD/YOCTO/imx-vpu-3.10.31-1.1.0-beta.bin" "$(pwd)/src/imx-vpu.bin"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
pull_bin "http://www.freescale.com/lgfiles/NMG/MAD/YOCTO/gpu-viv-g2d-3.10.17-1.0.2.bin" "$(pwd)/src/viv-g2d.bin"
pull_bin "http://www.freescale.com/lgfiles/NMG/MAD/YOCTO/libfslvpuwrap-1.0.54.bin" "$(pwd)/src/libfslvpuwrap.bin"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
pull_bin "http://www.freescale.com/lgfiles/NMG/MAD/YOCTO/gpu-viv-bin-mx6q-3.10.17-1.0.2-hfp.bin" "$(pwd)/src/gpu-viv.bin"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "vero-userland-osmc"
build_return=$?
if [ $build_return == 99 ]
then
	echo -e "Building package vero-userland-osmc"
	out=$(pwd)/files
        sed '/Depends/d' -i files-dev/DEBIAN/control
        sed '/Version/d' -i files-dev/DEBIAN/control
        VERSION_DEV=$(grep Version ${out}/DEBIAN/control)
        VERSION_NUM=$(echo $VERSION_DEV | awk {'print $2'})
        echo $VERSION_DEV >> files-dev/DEBIAN/control
        echo "Depends: vero-userland-osmc (=${VERSION_NUM})" >> files-dev/DEBIAN/control
	make clean
	update_sources
	handle_dep "libtool-bin"
	handle_dep "autoconf"
	handle_dep "automake"
	mkdir -p ${out}/opt/vero/lib
	mkdir -p files-dev/opt/vero/include
	pushd src
	install_patch "../patches" "all"
	cp -ar headers /
	cp -ar headers/include ../files-dev/opt/vero
	rm -rf headers > /dev/null 2>&1
	pushd imx-lib*
	sed -i */Makefile -e s/-O2/-O3/
	$BUILD PLATFORM=IMX6Q C_INCLUDE_PATH=/headers/include/ all
	make install PLATFORM=IMX6Q DEST_DIR=${out}
	if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
	popd
	sh firmware-imx.bin --auto-accept
	pushd firmware-imx*
	rm -rf firmware/ar3k
	rm -rf firmware/ath6k
	rm firmware/LICENCE.atheros_firmware
	rm firmware/README
	rm firmware/Android.mk
	mkdir -p ${out}/lib
	cp -ar firmware ${out}/lib
	popd
	sh viv-g2d.bin --auto-accept
	pushd gpu-viv-g2d*
	cp -ar usr/include ../../files-dev/opt/vero
	cp -ar usr/lib ${out}/opt/vero
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
	CFLAGS="-I../../files/usr/include -L../../files/usr/lib" ./autogen.sh --prefix=/opt/vero
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
	cp -ar usr/include ../../files-dev/opt/vero
	cp -ar usr/lib ${out}/opt/vero
	popd
	strip_libs
	popd
	cp -ar ${out}/usr/include files-dev/opt/vero/ # Remnants
	cp -ar ${out}/usr/lib ${out}/opt/vero/
	cp -ar ${out}/opt/vero/include files-dev/opt/vero
	rm -rf ${out}/usr/lib >/dev/null 2>&1
	rm -rf ${out}/usr/include >/dev/null 2>&1
	rm -rf ${out}/opt/vero/include >/dev/null 2>&1
	mkdir -p ${out}/etc/ld.so.conf.d
	echo "/opt/vero/lib" > files/etc/ld.so.conf.d/vero.conf
	rm -rf ${out}/opt/vero/share
	dpkg_build files/ vero-userland-osmc.deb
	dpkg_build files-dev vero-userland-dev-osmc.deb
	rm -rf /headers
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
