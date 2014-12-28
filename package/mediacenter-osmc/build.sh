# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

if [ "$1" == "rbp" ]
then
pull_source "https://github.com/popcornmix/xbmc/archive/helix_rbp_backports.tar.gz" "$(pwd)/kodi"
else
pull_source "https://github.com/xbmc/xbmc/archive/master.tar.gz" "$(pwd)/kodi"
fi
if [ $? != 0 ]; then echo -e "Error fetching Kodi source" && exit 1; fi
pull_source "https://github.com/opdenkamp/xbmc-pvr-addons/archive/master.tar.gz" "$(pwd)/kodi-pvr"
if [ $? != 0 ]; then echo -e "Error fetching Kodi PVR source" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "mediacenter-osmc"
if [ $? == 0 ]
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
	handle_dep "libcurl3"
	handle_dep "libcurl4-gnutls-dev"
	handle_dep "libdbus-1-dev"
	handle_dep "libenca-dev"
	handle_dep "libflac-dev"
	handle_dep "libfontconfig-dev"
	handle_dep "libfreetype6-dev"
	handle_dep "libfribidi-dev"
	handle_dep "libglew-dev"
	handle_dep "libiso9660-dev"
	handle_dep "libjasper-dev"
	handle_dep "libjpeg-dev"
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
	handle_dep "libpng-dev"
	handle_dep "libpulse-dev"
	handle_dep "libsdl-dev"
	handle_dep "libsdl-gfx1.2-dev"
	handle_dep "libsdl-image1.2-dev"
	handle_dep "libsdl-mixer1.2-dev"
	handle_dep "libsmbclient-dev"
	handle_dep "libsqlite3-dev"
	handle_dep "libssh-dev"
	handle_dep "libssl-dev"
	handle_dep "libtiff-dev"
	handle_dep "libtinyxml-dev"
	handle_dep "libtool"
	handle_dep "libudev-dev"
	handle_dep "libusb-dev"
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
	handle_dep "libsamplerate-dev"
	handle_dep "libmp3lame-dev"
	handle_dep "libltdl-dev"
	handle_dep "cmake"
	handle_dep "libgnutls28-dev"
	if [ "$1" == "rbp" ]
	then
		handle_dep "rbp-libcec-dev-osmc"
		handle_dep "rbp-libshairplay-dev-osmc"
		handle_dep "rbp-librtmp-dev-osmc"
		handle_dep "rbp-libnfs-dev-osmc"
		handle_dep "rbp-libafpclient-dev-osmc"
		handle_dep "rbp-userland-dev-osmc"
	fi
	sed '/Package/d' -i files/DEBIAN/control
	sed '/Depends/d' -i files/DEBIAN/control
	test "$1" == atv && echo "Package: atv-mediacenter-osmc" >> files/DEBIAN/control
	test "$1" == rbp && echo "Package: rbp-mediacenter-osmc" >> files/DEBIAN/control
	if [ "$1" == rbp ]
	then
		 pushd kodi/xbmc-helix*
	else
		 pushd kodi/xbmc-master*
	fi
	install_patch "../../patches" "all"
	test "$1" == atv && install_patch "../../patches" "atv"
	test "$1" == rbp && install_patch "../../patches" "rbp" && install_patch "../../patches" "lpr" && install_patch "../../patches/" "egl"
	./bootstrap
	# Apple TV configuration
	test "$1" == atv && \
	export CFLAGS="-I/usr/include/afpfs-ng -O3" && \
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
	test "$1" == rbp && \
	LIBRARY_PATH+=/opt/vc/lib && \
	export CFLAGS="-I/opt/vc/include -I/usr/include/afpfs-ng -I/opt/vc/include/interface -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host/linux -O3" && \
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
		--with-platform=raspberry-pi \
		--enable-optimizations \
		--enable-libcec \
		--enable-player=omxplayer
	if [ $? != 0 ]; then echo -e "Configure failed!" && umount /proc/ > /dev/null 2>&1 && exit 1; fi
	umount /proc/ > /dev/null 2>&1
	$BUILD
	if [ $? != 0 ]; then echo -e "Build failed!" && exit 1; fi
	make install DESTDIR=${out}
	popd
	pushd kodi-pvr/xbmc-pvr*
	./bootstrap
	./configure --prefix=/usr --enable-addons-with-dependencies
	if [ $? != 0 ]; then echo -e "Configure failed!" && exit 1; fi
	$BUILD
	make install DESTDIR=${out}
	if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
	popd
	rm -rf ${out}/usr/share/kodi/addons/service.*.versioncheck
	rm ${out}/usr/share/kodi/media/Splash.png
	strip ${out}/usr/lib/kodi/kodi.bin
	strip ${out}/usr/lib/kodi/addons/*/*.so
	strip ${out}/usr/lib/kodi/addons/pvr.*/*.pvr
	COMMON_DEPENDS="niceprioritypolicy-osmc, mediacenter-send-osmc, libssh-4, libavahi-client3, python, libsmbclient, libtiff5, libjpeg8, libsqlite3-0, libflac8, libtinyxml2.6.2, libogg0, libmad0, libmicrohttpd10, libjasper1, libyajl2, libmysqlclient18, libasound2, libxml2, liblzo2-2, libxslt1.1, libpng12-0, libsamplerate0, libtag1-vanilla, libfribidi0, libcdio13, libpcrecpp0, libfreetype6, libvorbisenc2, libcurl3-gnutls, libplist2, avahi-daemon, policykit-1, mediacenter-addon-osmc, mediacenter-skin-osmc"
	X86_DEPENDS="libcec-osmc, libshairplay-osmc, libnfs-osmc, libafpclient-osmc, librtmp-osmc, splash-osmc"
	test "$1" == atv && echo "Depends: ${COMMON_DEPENDS}, ${X86_DEPENDS}, libpulse0, libxrandr2, libsdl-image1.2, libglew1.10, libglu1-mesa, libcrystalhd3, firmware-crystalhd" >> files/DEBIAN/control
	test "$1" == rbp && echo "Depends: ${COMMON_DEPENDS}, libx11-6, rbp-libcec-osmc, rbp-libafpclient-osmc, rbp-libnfs-osmc, rbp-librtmp-osmc, rbp-libshairplay-osmc, rbp-userland-osmc, rbp-armmem-osmc, rbp-splash-osmc" >> files/DEBIAN/control
	cp patches/${1}-watchdog ${out}/usr/bin/mediacenter
	chmod +x ${out}/usr/bin/mediacenter
	fix_arch_ctl "files/DEBIAN/control"
	dpkg -b files/ mediacenter-osmc.deb
fi
teardown_env "${1}"
