# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

git_to_archive()
{
file_contents=$(cat $1)
if grep -q github.com $1
then
PKG_NAME=$(echo $file_contents | cut -f 1 -d " ")
GIT_REPO=$(echo $file_contents | cut -f 2 -d " ")
GIT_REV=$(echo $file_contents | cut -f 3 -d " ")
GIT_URL=$(echo ${GIT_REPO}/archive/${GIT_REV}.zip)
echo "${PKG_NAME} ${GIT_URL}" > $1
fi
}

. ../common.sh
if [ "$1" == "rbp1" ] || [ "$1" == "rbp2" ] || [ "$1" == "vero" ] || [ "$1" == "atv" ] || [ "$1" == "vero2" ]
then
pull_source "https://github.com/xbmc/xbmc/archive/6bcc4db1129fd65a3d97c9fa9a39437839622985.tar.gz" "$(pwd)/src"
API_VERSION="16"
else
pull_source "https://github.com/xbmc/xbmc/archive/master.tar.gz" "$(pwd)/kodi"
API_VERSION="17"
fi
if [ $? != 0 ]; then echo -e "Error fetching Kodi source" && exit 1; fi
# Build in native environment
BUILD_OPTS=$BUILD_OPTION_DEFAULTS
BUILD_OPTS=$(($BUILD_OPTS - $BUILD_OPTION_USE_CCACHE))
build_in_env "${1}" $(pwd) "mediacenter-osmc" "$BUILD_OPTS"
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
	handle_dep "default-jre-headless"
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
	handle_dep "libcurl4-openssl-dev"
	handle_dep "libdbus-1-dev"
	handle_dep "libfontconfig1-dev"
	handle_dep "libfreetype6-dev"
	handle_dep "libfribidi-dev"
	handle_dep "libgif-dev"
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
	handle_dep "libsmbclient-dev"
	handle_dep "libssh-dev"
	handle_dep "libavahi-client-dev"
	handle_dep "libssl-dev"
	handle_dep "libtiff5-dev"
	handle_dep "libtinyxml-dev"
	handle_dep "libtool"
	handle_dep "libudev-dev"
	handle_dep "libvorbis-dev"
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
	handle_dep "libtag1-dev"
	handle_dep "libsamplerate0-dev"
	handle_dep "libltdl-dev"
	handle_dep "cmake"
	handle_dep "libgnutls28-dev"
	handle_dep "git"
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
		handle_dep "armv6l-libplatform-dev-osmc"
		handle_dep "armv6l-libdcadec-dev-osmc"
		handle_dep "armv6l-libbluray-dev-osmc"
		handle_dep "armv6l-libsqlite-dev-osmc"
	fi
	if [ "$1" == "rbp2" ]
	then
		handle_dep "rbp2-libcec-dev-osmc"
		handle_dep "armv7-libshairplay-dev-osmc"
		handle_dep "armv7-librtmp-dev-osmc"
		handle_dep "armv7-libnfs-dev-osmc"
		handle_dep "armv7-libplatform-dev-osmc"
		handle_dep "armv7-libdcadec-dev-osmc"
		handle_dep "armv7-libbluray-dev-osmc"
		handle_dep "armv7-libsqlite-dev-osmc"
	fi
	if [ "$1" == "vero" ]
	then
		handle_dep "vero-libcec-dev-osmc"
		handle_dep "armv7-libshairplay-dev-osmc"
		handle_dep "armv7-librtmp-dev-osmc"
		handle_dep "armv7-libnfs-dev-osmc"
		handle_dep "armv7-libplatform-dev-osmc"
		handle_dep "armv7-libdcadec-dev-osmc"
		handle_dep "armv7-libbluray-dev-osmc"
		handle_dep "armv7-libsqlite-dev-osmc"
	fi
        if [ "$1" == "vero2" ]
        then
		handle_dep "vero2-userland-dev-osmc"
                handle_dep "armv7-libshairplay-dev-osmc"
                handle_dep "armv7-librtmp-dev-osmc"
                handle_dep "armv7-libnfs-dev-osmc"
                handle_dep "armv7-libplatform-dev-osmc"
                handle_dep "armv7-libdcadec-dev-osmc"
                handle_dep "armv7-libbluray-dev-osmc"
                handle_dep "armv7-libsqlite-dev-osmc"
        fi
	if [ "$1" == "atv" ] # later we change this to if_x11..
	then
		handle_dep "i386-libcec-dev-osmc"
		handle_dep "i386-libshairplay-dev-osmc"
		handle_dep "i386-librtmp-dev-osmc"
		handle_dep "i386-libnfs-dev-osmc"
		handle_dep "i386-libplatform-dev-osmc"
		handle_dep "i386-libdcadec-dev-osmc"
		handle_dep "i386-libbluray-dev-osmc"
		handle_dep "i386-libsqlite-dev-osmc"
		handle_dep "libglew-dev"
		handle_dep "libsdl1.2-dev"
		handle_dep "libsdl-gfx1.2-dev"
		handle_dep "libsdl-image1.2-dev"
		handle_dep "libsdl-mixer1.2-dev"
		handle_dep "i386-libcrystalhd-dev-osmc"
		handle_dep "xserver-xorg-dev"
		handle_dep "libxrandr-dev"
	fi
	sed '/Package/d' -i files/DEBIAN/control
	sed '/Depends/d' -i files/DEBIAN/control
	sed '/Package/d' -i files-debug/DEBIAN/control
	sed '/Depends/d' -i files-debug/DEBIAN/control
	sed '/Version/d' -i files-debug/DEBIAN/control
	echo "Package: ${1}-mediacenter-osmc" >> files/DEBIAN/control
	echo "Package: ${1}-mediacenter-debug-osmc" >> files-debug/DEBIAN/control
	VERSION_DBG=$(grep Version ${out}/DEBIAN/control)
	VERSION_NUM=$(echo $VERSION_DBG | awk {'print $2'})
	echo $VERSION_DBG >> files-debug/DEBIAN/control
	echo "Depends: ${1}-mediacenter-osmc (=${VERSION_NUM})" >> files-debug/DEBIAN/control
	pushd src/xbmc-*
	install_patch "../../patches" "all"
	test "$1" == atv && install_patch "../../patches" "atv"
	if [ "$1" == "rbp1" ] || [ "$1" == "rbp2" ]
	then
		install_patch "../../patches" "rbp"
	fi
	if [ "$1" == "rbp1" ] || [ "$1" == "rbp2" ] || [ "$1" == "vero" ]; then install_patch "../../patches" "arm"; fi
	test "$1" == vero && install_patch "../../patches" "vero"
	./bootstrap
	# Apple TV configuration
	test "$1" == atv && \
	COMPFLAGS="-O3 -fomit-frame-pointer -I/usr/include/libcrystalhd  -Wl,-rpath=/usr/osmc/lib -L/usr/osmc/lib " && \
	export CFLAGS+=${COMPFLAGS} && \
	export CXXFLAGS+=${COMPFLAGS} && \
	export CPPFLAGS+=${COMPFLAGS} && \
	export LDFLAGS="" & \
	./configure \
		--prefix=/usr \
		--disable-vtbdecoder \
		--disable-vaapi \
		--disable-vdpau \
		--disable-pulse \
		--disable-projectm \
		--enable-x11 \
		--disable-openmax \
		--enable-optical-drive \
		--enable-libbluray \
		--enable-dvdcss \
                --enable-dvdcss \
                --disable-joystick \
                --disable-debug \
                --enable-libcec \
		--disable-optimizations \
		--enable-crystalhd
	# Raspberry Pi Configuration
	if [ "$1" == "rbp1" ]
	then
		PIDEV="raspberry-pi";
		COMPFLAGS="-O3 -fomit-frame-pointer "
	fi
	if [ "$1" == "rbp2" ]
	then
		PIDEV="raspberry-pi2"
		COMPFLAGS="-mcpu=cortex-a7 -mtune=cortex-a7 -mfloat-abi=hard -O3 -mfpu=neon-vfpv4 -fomit-frame-pointer "
	fi
	if [ "$1" == "rbp1" ] || [ "$1" == "rbp2" ]; then
	LIBRARY_PATH+=/opt/vc/lib && \
	COMPFLAGS+="-I/opt/vc/include -I/opt/vc/include/interface -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host/linux -Wl,-rpath=/usr/osmc/lib -L/usr/osmc/lib " && \
	export CFLAGS+=${COMPFLAGS} && \
	export CXXFLAGS+=${COMPFLAGS} && \
	export CPPFLAGS+=${COMPFLAGS} && \
	export LDFLAGS="-L/opt/vc/lib" && \
	./configure \
		--prefix=/usr \
		--enable-gles \
		--disable-x11 \
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
		--disable-optimizations \
		--enable-libcec \
		--enable-player=omxplayer \
		--build=arm-linux
	fi
	if [ "$1" == "vero" ]; then
	LIBRARY_PATH+="/opt/vero/lib" && \
	COMPFLAGS="-I/opt/vero/include -Wl,-rpath=/usr/osmc/lib -L/usr/osmc/lib " && \
	export CFLAGS+=${COMPFLAGS} && \
	export CXXFLAGS+=${COMPFLAGS} && \
	export CPPFLAGS+=${COMPFLAGS} && \
	export LDFLAGS="-L/opt/vero/lib" && \
	./configure \
		--prefix=/usr \
		--disable-x11 \
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
		--disable-optimizations \
		--with-platform=vero \
		--build=arm-linux
	fi
        if [ "$1" == "vero2" ]; then
        LIBRARY_PATH+="/opt/vero2/lib" && \
        COMPFLAGS="-I/opt/vero2/include -Wl,-rpath=/usr/osmc/lib -L/usr/osmc/lib " && \
        export CFLAGS+=${COMPFLAGS} && \
        export CXXFLAGS+=${COMPFLAGS} && \
        export CPPFLAGS+=${COMPFLAGS} && \
        export LDFLAGS="-L/opt/vero2/lib" && \
        ./configure \
                --prefix=/usr \
                --disable-x11 \
                --disable-openmax \
                --disable-vdpau \
                --disable-vaapi \
                --enable-gles \
                --enable-codec=aml \
                --disable-libcec \
                --disable-debug \
                --disable-texturepacker \
                --enable-optical-drive \
                --enable-dvdcss \
                --enable-libbluray \
                --disable-joystick \
                --disable-vtbdecoder \
                --disable-pulse \
                --disable-projectm \
                --disable-optimizations \
                --with-platform=vero2 \
                --build=arm-linux
        fi
	if [ $? != 0 ]; then echo -e "Configure failed!" && umount /proc/ > /dev/null 2>&1 && exit 1; fi
	umount /proc/ > /dev/null 2>&1
	$BUILD
	if [ $? != 0 ]; then echo -e "Build failed!" && exit 1; fi
	make install DESTDIR=${out}
	# Binary addons
	for file in project/cmake/addons/addons/*/*.txt
	do
	    git_to_archive "$file"
	done
	pushd project/cmake/addons/
	mkdir build
	cd build
	cmake -DCMAKE_INSTALL_DESTDIR=/usr -DCMAKE_INSTALL_PREFIX=${out}/usr/ -DBUILD_DIR=$(pwd) ../ -DOVERRIDE_PATHS=1
	if [ $? != 0 ]; then echo "Configuring binary addons failed" && exit 1; fi
	cd ../
	$BUILD kodiplatform_DIR=$(pwd) -C build/
	if [ $? != 0 ]; then echo "Building binary addons failed" && exit 1; fi
	popd
        # Languages
        mkdir languages/
        pushd languages
        if [ "$API_VERSION" = "16" ]; then api_name="jarvis"; fi
        if [ "$API_VERSION" = "17" ]; then api_name="tbc"; fi
        base_url="http://mirror.us.leaseweb.net/xbmc/addons/${api_name}"
	handle_dep "wget" # We do not usually use wget in the build environment
        languages=$(wget ${base_url} -O- | grep resource.language. | sed -e 's/<a/\n<a/g' | sed -e 's/<a .*href=['"'"'"]//' -e 's/["'"'"'].*$//' -e '/^$/ d' | sed '/tr/d' | sed 's/resource.language.//' | tr -d /)
        if [ $? != 0 ]; then echo "Can't get list of languages" && exit 1; fi
        for language in ${languages}
        do
            echo "Downloading language file for ${language}"
            language_file=$(wget ${base_url}/resource.language.${language} -O- | grep -o '<a .*href=.*>' | sed -e 's/<a/\n<a/g' | sed -e 's/<a .*href=['"'"'"]//' -e 's/["'"'"'].*$//' -e '/^$/ d' | tail -n 1)
            if [ $? != 0 ]; then echo "Can't determine language file" && exit 1; fi
	    wget ${base_url}/resource.language.${language}/${language_file}
            if [ $? != 0 ]; then echo "Couldn't download language file ${language_file}" && exit 1; fi
	    unzip ${language_file}
	    rm ${language_file}
        done
        cp -ar resource.language.* ${out}/usr/share/kodi/addons
	popd
	popd
	rm -rf ${out}/usr/share/kodi/addons/service.*.versioncheck
	mkdir -p files-debug/usr/lib/kodi
	cp -ar ${out}/usr/lib/kodi/kodi.bin files-debug/usr/lib/kodi/kodi.bin
	strip -s ${out}/usr/lib/kodi/kodi.bin
	COMMON_DEPENDS="niceprioritypolicy-osmc, mediacenter-send-osmc, libssh-4, libavahi-client3, python, python-imaging, python-unidecode, libsmbclient, libtiff5, libjpeg62-turbo, libsqlite3-0, libtinyxml2.6.2, libogg0, libmad0, libmicrohttpd10, libjasper1, libyajl2, libmysqlclient18, libasound2, libxml2, liblzo2-2, libxslt1.1, libpng12-0, libsamplerate0, libtag1-vanilla, libfribidi0, libgif4, libcdio13, libpcrecpp0, libfreetype6, libvorbis0a, libvorbisenc2, libass5, libcurl3, libssl1.0.0, libplist2, avahi-daemon, policykit-1, mediacenter-addon-osmc (>= 3.0.39), mediacenter-skin-osmc, diskmount-osmc (>= 1.2.9)"
	test "$1" == atv && echo "Depends: ${COMMON_DEPENDS}, i386-libcec-osmc, i386-libnfs-osmc, i386-librtmp-osmc, i386-libshairplay-osmc, i386-libbluray-osmc, i386-libsqlite-osmc, libxrandr2, libsdl-image1.2, libglew1.10, libglu1-mesa, i386-libcrystalhd-osmc, xserver-xorg-core, xserver-xorg, xinit, xfonts-base, x11-xserver-utils, xauth, alsa-utils, xserver-xorg-video-nvidia-legacy-304xx, nvidia-xconfig" >> files/DEBIAN/control
	test "$1" == rbp1 && echo "Depends: ${COMMON_DEPENDS}, rbp1-libcec-osmc, armv6l-libnfs-osmc, armv6l-librtmp-osmc, armv6l-libshairplay-osmc, armv6l-libbluray-osmc, armv6l-libsqlite-osmc, rbp-userland-osmc, armv6l-splash-osmc" >> files/DEBIAN/control
	test "$1" == rbp2 && echo "Depends: ${COMMON_DEPENDS}, rbp2-libcec-osmc, armv7-libnfs-osmc, armv7-librtmp-osmc, armv7-libshairplay-osmc, armv7-libbluray-osmc, armv7-libsqlite-osmc, rbp-userland-osmc, armv7-splash-osmc" >> files/DEBIAN/control
	test "$1" == vero && echo "Depends: ${COMMON_DEPENDS}, vero-libcec-osmc, armv7-libnfs-osmc, armv7-librtmp-osmc, armv7-libshairplay-osmc, armv7-libbluray-osmc, armv7-libsqlite-osmc, vero-userland-osmc, armv7-splash-osmc" >> files/DEBIAN/control
	cp patches/${1}-watchdog ${out}/usr/bin/mediacenter
	cp patches/${1}-advancedsettings.xml ${out}/usr/share/kodi/system/advancedsettings.xml
	chmod +x ${out}/usr/bin/mediacenter
	test "$1" == vero && cp patches/${1}-hdmi-trace ${out}/usr/bin/hdmi-trace && chmod +x ${out}/usr/bin/hdmi-trace
	fix_arch_ctl "files/DEBIAN/control"
	fix_arch_ctl "files-debug/DEBIAN/control"
	dpkg_build files/ ${1}-mediacenter-osmc.deb
	dpkg_build files-debug/ ${1}-mediacenter-debug-osmc.deb
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
