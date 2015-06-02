# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh
if [ "$1" == "rbp1" ] || [ "$1" == "rbp2" ] || [ "$1" == "vero" ]
then
pull_source "https://github.com/xbmc/xbmc/archive/cd21822005be72a7ad3a1fa59a88ea5362dc5109.tar.gz" "$(pwd)/src"
else
pull_source "https://github.com/xbmc/xbmc/archive/master.tar.gz" "$(pwd)/kodi"
fi
if [ $? != 0 ]; then echo -e "Error fetching Kodi source" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "mediacenter-osmc"
build_return=$?
if [ $build_return == 99 ]
then
	echo -e "Building package Kodi"
	out=$(pwd)/files
	make clean
	mount -t proc proc /proc >/dev/null 2>&1
	update_sources
	handle_dep "autopoint"
	handle_dep "automake"
	handle_dep "bison"
	handle_dep "make"
	handle_dep "curl"
	handle_dep "cvs"
	handle_dep "default-jre"
	handle_dep "fp-compiler"
	handle_dep "gawk"
	handle_dep "gdc"
	handle_dep "gettext"
	handle_dep "gperf"
	handle_dep "libasound2-dev"
	handle_dep "libass-dev"
	handle_dep "libboost-dev"
	handle_dep "libboost-thread-dev"
	handle_dep "libbz2-dev"
	handle_dep "libcap-dev"
	handle_dep "libcdio-dev"
	handle_dep "libcurl4-gnutls-dev"
	handle_dep "libdbus-1-dev"
	handle_dep "libenca-dev"
	handle_dep "libflac-dev"
	handle_dep "libfontconfig1-dev"
	handle_dep "libfreetype6-dev"
	handle_dep "libfribidi-dev"
	handle_dep "libglew-dev"
	handle_dep "libiso9660-dev"
	handle_dep "libjasper-dev"
	handle_dep "libjpeg62-turbo-dev"
	handle_dep "liblzo2-dev"
	handle_dep "libmad0-dev"
	handle_dep "libmicrohttpd-dev"
	handle_dep "libmodplug-dev"
	handle_dep "libmpeg2-4-dev"
	handle_dep "libmpeg3-dev"
	handle_dep "libmysqlclient-dev"
	handle_dep "libogg-dev"
	handle_dep "libpcre3-dev"
	handle_dep "libplist-dev"
	handle_dep "libpng12-dev"
	handle_dep "libsdl1.2-dev"
	handle_dep "libsdl-gfx1.2-dev"
	handle_dep "libsdl-image1.2-dev"
	handle_dep "libsdl-mixer1.2-dev"
	handle_dep "libsmbclient-dev"
	handle_dep "libsqlite3-dev"
	handle_dep "libssh-dev"
	handle_dep "libssl-dev"
	handle_dep "libtiff5-dev"
	handle_dep "libtinyxml-dev"
	handle_dep "libtool"
	handle_dep "libudev-dev"
	handle_dep "libvorbisenc2"
	handle_dep "libxml2-dev"
	handle_dep "libxmu-dev"
	handle_dep "libxslt1-dev"
	handle_dep "libxt-dev"
	handle_dep "libyajl-dev"
	handle_dep "nasm"
	handle_dep "pmount"
	handle_dep "python-dev"
	handle_dep "python-imaging"
	handle_dep "python-sqlite"
	handle_dep "swig"
	handle_dep "unzip"
	handle_dep "yasm"
	handle_dep "zip"
	handle_dep "zlib1g-dev"
	handle_dep "libbluray-dev"
	handle_dep "libtag1-dev"
	handle_dep "libsamplerate0-dev"
	handle_dep "libmp3lame-dev"
	handle_dep "libltdl-dev"
	handle_dep "cmake"
	handle_dep "libgnutls28-dev"
	if [ "$1" == "rbp1" ] || [ "$1" == "rbp2" ]
	then
		handle_dep "rbp-userland-dev-osmc"
	fi
	if [ "$1" == "vero" ]
	then
		handle_dep "vero-userland-dev-osmc"
	fi
	if [ "$1" == "rbp1" ]
	then
		handle_dep "rbp1-libcec-dev-osmc"
		handle_dep "armv6l-libshairplay-dev-osmc"
		handle_dep "armv6l-librtmp-dev-osmc"
		handle_dep "armv6l-libnfs-dev-osmc"
		handle_dep "armv6l-libafpclient-dev-osmc"
	fi
	if [ "$1" == "rbp2" ]
	then
		handle_dep "rbp2-libcec-dev-osmc"
		handle_dep "armv7-libshairplay-dev-osmc"
		handle_dep "armv7-librtmp-dev-osmc"
		handle_dep "armv7-libnfs-dev-osmc"
		handle_dep "armv7-libafpclient-dev-osmc"
	fi
	if [ "$1" == "vero" ]
	then
		handle_dep "vero-libcec-dev-osmc"
		handle_dep "armv7-libshairplay-dev-osmc"
		handle_dep "armv7-librtmp-dev-osmc"
		handle_dep "armv7-libnfs-dev-osmc"
		handle_dep "armv7-libafpclient-dev-osmc"
	fi
	sed '/Package/d' -i files/DEBIAN/control
	sed '/Depends/d' -i files/DEBIAN/control
	echo "Package: ${1}-mediacenter-osmc" >> files/DEBIAN/control
	pushd src/xbmc-*
	install_patch "../../patches" "all"
	test "$1" == atv && install_patch "../../patches" "atv"
	if [ "$1" == "rbp1" ] || [ "$1" == "rbp2" ]
	then
		install_patch "../../patches" "rbp"
		test "$1" == rbp1 && install_patch "../../patches" "lpr"
	fi
	if [ "$1" == "rbp1" ] || [ "$1" == "rbp2" ] || [ "$1" == "vero" ]; then install_patch "../../patches" "arm"; fi
	test "$1" == vero && install_patch "../../patches" "vero"
	./bootstrap
	# Apple TV configuration
	test "$1" == atv && \
	export CFLAGS="-I/usr/include/afpfs-ng" && \
	export CXXFLAGS=$CFLAGS && \
	export CPPFLAGS=$CFLAGS && \
	./configure \
		--prefix=/usr \
		--disable-vtbdecoder \
		--disable-vaapi \
		--disable-vdpau \
		--disable-pulse \
		--disable-projectm
	# Raspberry Pi Configuration
	if [ "$1" == "rbp1" ]; then PIDEV="raspberry-pi"; fi
	if [ "$1" == "rbp2" ]
	then
		PIDEV="raspberry-pi2"
		CFLAGS="-mcpu=cortex-a7 -mfpu=neon-vfpv4 "
	fi
	if [ "$1" == "rbp1" ] || [ "$1" == "rbp2" ]; then
	LIBRARY_PATH+=/opt/vc/lib && \
	export CFLAGS+="-I/opt/vc/include -I/usr/include/afpfs-ng -I/opt/vc/include/interface -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host/linux" && \
	export CXXFLAGS=$CFLAGS && \
	export CPPFLAGS=$CFLAGS && \
	export LDFLAGS="-L/opt/vc/lib" && \
	./configure \
		--prefix=/usr \
		--enable-gles \
		--disable-sdl \
		--disable-x11 \
		--disable-xrandr \
		--disable-openmax \
		--enable-optical-drive \
		--enable-libbluray \
		--enable-dvdcss \
		--disable-joystick \
		--disable-debug \
		--disable-vtbdecoder \
		--disable-vaapi \
		--disable-vdpau \
		--disable-pulse \
		--disable-projectm \
		--with-platform=$PIDEV \
		--enable-optimizations \
		--enable-libcec \
		--enable-player=omxplayer \
		--build=arm-linux
	fi
	if [ "$1" == "vero" ]; then
	LIBRARY_PATH+="/opt/vero/lib" && \
	export CFLAGS+="-mcpu=cortex-a9 -mfpu=neon -I/usr/include/afpfs-ng -I/opt/vero/include" && \
	export CXXFLAGS=$CFLAGS && \
	export CPPFLAGS=$CFLAGS && \
	export LDFLAGS="-L/opt/vero/lib" && \
	./configure \
		--prefix=/usr \
		--disable-x11 \
		--disable-sdl \
		--disable-xrandr \
		--disable-openmax \
		--disable-vdpau \
		--disable-vaapi \
		--enable-gles \
		--enable-codec=imxvpu \
		--enable-libcec \
		--disable-debug \
		--disable-texturepacker \
		--enable-optical-drive \
		--enable-dvdcss \
		--enable-libbluray \
		--disable-joystick \
		--disable-vtbdecoder \
		--disable-pulse \
		--disable-projectm \
		--enable-optimizations \
		--with-platform=vero \
		--build=arm-linux
	fi
	if [ $? != 0 ]; then echo -e "Configure failed!" && umount /proc/ > /dev/null 2>&1 && exit 1; fi
	# Hack:
	sed -i Makefile.include -e s/-O2//g
	umount /proc/ > /dev/null 2>&1
	$BUILD
	if [ $? != 0 ]; then echo -e "Build failed!" && exit 1; fi
	make install DESTDIR=${out}
        gcc addon-compiler.c -o addon-compiler
        mv addon-compiler ${out}/usr/bin
        popd
	rm -rf ${out}/usr/share/kodi/addons/service.*.versioncheck
	strip ${out}/usr/lib/kodi/kodi.bin
	COMMON_DEPENDS="niceprioritypolicy-osmc, mediacenter-send-osmc, libssh-4, libavahi-client3, python, python-imaging, python-unidecode, libsmbclient, libbluray1, libtiff5, libjpeg62-turbo, libsqlite3-0, libflac8, libtinyxml2.6.2, libogg0, libmad0, libmicrohttpd10, libjasper1, libyajl2, libmysqlclient18, libasound2, libxml2, liblzo2-2, libxslt1.1, libpng12-0, libsamplerate0, libtag1-vanilla, libfribidi0, libcdio13, libpcrecpp0, libfreetype6, libvorbisenc2, libass5, libcurl3-gnutls, libplist2, avahi-daemon, policykit-1, mediacenter-addon-osmc, mediacenter-skin-osmc, diskmount-osmc (>= 1.2.9)"
	test "$1" == atv && echo "Depends: ${COMMON_DEPENDS}, ${X86_DEPENDS}, libxrandr2, libsdl-image1.2, libglew1.10, libglu1-mesa, libcrystalhd3, firmware-crystalhd" >> files/DEBIAN/control
	test "$1" == rbp1 && echo "Depends: ${COMMON_DEPENDS}, libx11-6, rbp1-libcec-osmc, armv6l-libafpclient-osmc, armv6l-libnfs-osmc, armv6l-librtmp-osmc, armv6l-libshairplay-osmc, rbp-userland-osmc, armv6l-splash-osmc" >> files/DEBIAN/control
	test "$1" == rbp2 && echo "Depends: ${COMMON_DEPENDS}, libx11-6, rbp2-libcec-osmc, armv7-libafpclient-osmc, armv7-libnfs-osmc, armv7-librtmp-osmc, armv7-libshairplay-osmc, rbp-userland-osmc, armv7-splash-osmc" >> files/DEBIAN/control
	test "$1" == vero && echo "Depends: ${COMMON_DEPENDS}, libx11-6, vero-libcec-osmc, armv7-libafpclient-osmc, armv7-libnfs-osmc, armv7-librtmp-osmc, armv7-libshairplay-osmc, vero-userland-osmc, armv7-splash-osmc" >> files/DEBIAN/control
	cp patches/${1}-watchdog ${out}/usr/bin/mediacenter
	cp patches/${1}-advancedsettings.xml ${out}/usr/share/kodi/system/advancedsettings.xml
	chmod +x ${out}/usr/bin/mediacenter
	fix_arch_ctl "files/DEBIAN/control"
	dpkg_build files/ ${1}-mediacenter-osmc.deb
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
