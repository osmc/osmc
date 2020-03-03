# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh
if [ "$1" == "rbp1" ] || [ "$1" == "rbp2" ] || [ "$1" == "pc" ] || [ "$1" == "vero2" ] || [ "$1" == "vero3" ]
then
pull_source "https://github.com/xbmc/xbmc/archive/8e967df9218279618bcbfa8a898d8f80f7b4e449.tar.gz" "$(pwd)/src"
API_VERSION="18"
else
pull_source "https://github.com/xbmc/xbmc/archive/master.tar.gz" "$(pwd)/kodi"
API_VERSION="19"
fi
if [ $? != 0 ]; then echo -e "Error fetching Kodi source" && exit 1; fi
# Build in native environment
BUILD_OPTS=$BUILD_OPTION_DEFAULTS
BUILD_OPTS=$(($BUILD_OPTS - $BUILD_OPTION_USE_CCACHE))
if [ "$1" == "rbp1" ] || [ "$1" == "rbp2" ] || [ "$1" == "vero2" ] || [ "$1" == "vero3" ]
then
    BUILD_OPTS=$(($BUILD_OPTS + $BUILD_OPTION_NEEDS_SWAP))
fi
build_in_env "${1}" $(pwd) "mediacenter-osmc" "$BUILD_OPTS"
build_return=$?
if [ $build_return == 99 ]
then
	# Fix Kodi CMake issues where local dependencies are introduced in the toolchain
	rm -rf /usr/local/include/* >/dev/null 2>&1
	rm -rf /usr/local/lib/* >/dev/null 2>&1
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
	handle_dep "gawk"
	handle_dep "gdc"
	handle_dep "gettext"
	handle_dep "gperf"
	handle_dep "liblirc-dev"
	handle_dep "libasound2-dev"
	handle_dep "libbz2-dev"
	handle_dep "libcap-dev"
	handle_dep "libcdio-dev"
	handle_dep "libcurl4-openssl-dev"
	handle_dep "libdbus-1-dev"
	handle_dep "libfontconfig1-dev"
	handle_dep "libfreetype6-dev"
	handle_dep "libfribidi-dev"
	handle_dep "libfstrcmp-dev"
	handle_dep "libinput-dev"
	handle_dep "libgif-dev"
	handle_dep "libiso9660-dev"
	handle_dep "libjpeg62-turbo-dev"
	handle_dep "liblzo2-dev"
	handle_dep "libmad0-dev"
	handle_dep "libmicrohttpd-dev"
	handle_dep "libmodplug-dev"
	handle_dep "libmariadbclient-dev-compat"
	handle_dep "libpcre3-dev"
	handle_dep "libplist-dev"
	handle_dep "libpng-dev"
	handle_dep "libsmbclient-dev"
	handle_dep "libssh-dev"
	handle_dep "libavahi-client-dev"
	handle_dep "libssl-dev"
	handle_dep "libtinyxml-dev"
	handle_dep "libtool"
	handle_dep "libudev-dev"
	handle_dep "libvorbis-dev"
	handle_dep "libxml2-dev"
	handle_dep "libxmu-dev"
	handle_dep "libxslt1-dev"
	handle_dep "libxt-dev"
	handle_dep "libyajl-dev"
        handle_dep "libxkbcommon-dev"
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
	handle_dep "libgnutls28-dev"
	handle_dep "git"
	handle_dep "uuid-dev"
	handle_dep "libcrossguid-dev"
	handle_dep "cmake"
	handle_dep "rapidjson-dev"
        handle_dep "libgcrypt11-dev"
	if [ "$1" == "rbp1" ] || [ "$1" == "rbp2" ]
	then
		handle_dep "rbp-userland-dev-osmc"
	fi
	if [ "$1" == "rbp1" ]
	then
		handle_dep "rbp1-libcec-dev-osmc"
		handle_dep "armv6l-libshairplay-dev-osmc"
		handle_dep "armv6l-librtmp-dev-osmc"
		handle_dep "armv6l-libnfs-dev-osmc"
		handle_dep "armv6l-libplatform-dev-osmc"
		handle_dep "armv6l-libbluray-dev-osmc"
		handle_dep "armv6l-libsqlite-dev-osmc"
		handle_dep "armv6l-libass-dev-osmc"
	fi
	if [ "$1" == "rbp2" ]
	then
		handle_dep "rbp2-libcec-dev-osmc"
		handle_dep "armv7-libshairplay-dev-osmc"
		handle_dep "armv7-librtmp-dev-osmc"
		handle_dep "armv7-libnfs-dev-osmc"
		handle_dep "armv7-libplatform-dev-osmc"
		handle_dep "armv7-libbluray-dev-osmc"
		handle_dep "armv7-libsqlite-dev-osmc"
		handle_dep "armv7-libass-dev-osmc"
	fi
        if [ "$1" == "vero2" ]
        then
		handle_dep "vero2-libcec-dev-osmc"
		handle_dep "vero2-userland-dev-osmc"
		handle_dep "vero2-libamcodec-dev-osmc"
                handle_dep "armv7-libshairplay-dev-osmc"
                handle_dep "armv7-librtmp-dev-osmc"
                handle_dep "armv7-libnfs-dev-osmc"
                handle_dep "armv7-libplatform-dev-osmc"
                handle_dep "armv7-libbluray-dev-osmc"
                handle_dep "armv7-libsqlite-dev-osmc"
		handle_dep "armv7-libass-dev-osmc"
        fi
	if [ "$1" == "vero3" ]
	then
		handle_dep "vero3-libcec-dev-osmc"
		handle_dep "vero3-userland-dev-osmc"
		handle_dep "vero3-libamcodec-dev-osmc"
		handle_dep "armv7-libshairplay-dev-osmc"
                handle_dep "armv7-librtmp-dev-osmc"
                handle_dep "armv7-libnfs-dev-osmc"
                handle_dep "armv7-libplatform-dev-osmc"
                handle_dep "armv7-libbluray-dev-osmc"
                handle_dep "armv7-libsqlite-dev-osmc"
                handle_dep "armv7-libass-dev-osmc"
	fi
	if [ "$1" == "pc" ]
	then
		handle_dep "amd64-libshairplay-dev-osmc"
		handle_dep "amd64-librtmp-dev-osmc"
		handle_dep "amd64-libnfs-dev-osmc"
		handle_dep "amd64-libplatform-dev-osmc"
		handle_dep "amd64-libbluray-dev-osmc"
		handle_dep "amd64-libsqlite-dev-osmc"
		handle_dep "xserver-xorg-dev"
		handle_dep "libxrandr-dev"
		handle_dep "x11proto-randr-dev"
		handle_dep "libegl1-mesa-dev"
		handle_dep "libglew-dev"
		handle_dep "amd64-libass-dev-osmc"
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
	test "$1" == pc && install_patch "../../patches" "pc"
	if [ "$1" == "rbp1" ] || [ "$1" == "rbp2" ]
	then
		install_patch "../../patches" "rbp"
	fi
	if [ "$1" == "rbp1" ] || [ "$1" == "rbp2" ] || [ "$1" == "vero2" ] || [ "$1" == "vero3" ]; then install_patch "../../patches" "arm"; fi
	test "$1" == vero2 && install_patch "../../patches" "vero2"
	test "$1" == vero3 && install_patch "../../patches" "vero3"
        if [ "$1" == "pc" ]; then
        COMPFLAGS="-O3 -fomit-frame-pointer -Wl,-rpath=/usr/osmc/lib -L/usr/osmc/lib " && \
        export CFLAGS+=${COMPFLAGS} && \
        export CXXFLAGS+=${COMPFLAGS} && \
        export CPPFLAGS+=${COMPFLAGS} && \
        export LDFLAGS="" && \
        cmake -DCMAKE_INSTALL_PREFIX=/usr \
            -DCMAKE_INSTALL_LIBDIR=/usr/lib \
            -DGBM_RENDER_SYSTEM=gl \
            -DCMAKE_INCLUDE_PATH=/usr/osmc/include \
            -DCMAKE_LIBRARY_PATH=/usr/osmc/lib \
            -DENABLE_OPTICAL=1 \
            -DENABLE_DVDCSS=1 \
            -DCORE_SYSTEM_NAME=linux \
            -DCORE_PLATFORM_NAME=gbm \
            -DENABLE_APP_AUTONAME=OFF \
            -DENABLE_INTERNAL_FMT=ON \
            -DENABLE_INTERNAL_FLATBUFFERS=ON \
         .
	fi
	# Raspberry Pi Configuration
	if [ "$1" == "rbp1" ]
	then
		CPU="arm1176jzf-s"
		COMPFLAGS="-O3 -fomit-frame-pointer -mfpu=vfp "
	fi
	if [ "$1" == "rbp2" ]
	then
		CPU="cortex-a7"
		COMPFLAGS="-mcpu=cortex-a7 -mtune=cortex-a7 -mfloat-abi=hard -O3 -mfpu=neon-vfpv4 -fomit-frame-pointer "
	fi
	if [ "$1" == "rbp1" ] || [ "$1" == "rbp2" ]; then
	LIBRARY_PATH+=/opt/vc/lib && \
	COMPFLAGS+="-I/opt/vc/include -I/opt/vc/include/interface -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host/linux -Wl,-rpath=/usr/osmc/lib -L/usr/osmc/lib " && \
	export CFLAGS+=${COMPFLAGS} && \
	export CXXFLAGS+=${COMPFLAGS} && \
	export CPPFLAGS+=${COMPFLAGS} && \
	export LDFLAGS="-L/opt/vc/lib" && \
        cmake -DCMAKE_INSTALL_PREFIX=/usr \
            -DCMAKE_INSTALL_LIBDIR=/usr/lib \
            -DCMAKE_PREFIX_PATH=/opt/vc \
            -DCMAKE_INCLUDE_PATH=/usr/osmc/include \
            -DCMAKE_LIBRARY_PATH=/usr/osmc/lib \
            -DASS_INCLUDE_DIR=/usr/osmc/lib \
            -DSHAIRPLAY_INCLUDE_DIR=/usr/osmc/include/shairplay/ \
            -DENABLE_OPENGLES=ON \
            -DENABLE_OPENGL=OFF \
            -DENABLE_OPTICAL=1 \
            -DENABLE_DVDCSS=1 \
            -DCORE_SYSTEM_NAME=linux \
            -DCORE_PLATFORM_NAME=rbpi \
            -DWITH_ARCH=arm \
            -DWITH_CPU=${CPU} \
            -DENABLE_APP_AUTONAME=OFF \
            -DENABLE_INTERNAL_FMT=ON \
            -DENABLE_INTERNAL_FLATBUFFERS=ON \
            -DENABLE_MDNS=OFF \
            -DENABLE_BLUETOOTH=OFF \
            -DENABLE_PULSEAUDIO=OFF \
            -DENABLE_LCMS2=OFF \
            -DENABLE_SNDIO=OFF \
            -DENABLE_MARIADBCLIENT=OFF \
        .
	fi
        if [ "$1" == "vero2" ]; then
        LIBRARY_PATH+="/opt/vero2/lib" && \
        COMPFLAGS="-I/opt/vero2/include -Wl,-rpath=/usr/osmc/lib -L/usr/osmc/lib " && \
        export CFLAGS+=${COMPFLAGS} && \
        export CXXFLAGS+=${COMPFLAGS} && \
        export CPPFLAGS+=${COMPFLAGS} && \
        export LDFLAGS="-L/opt/vero2/lib" && \
    	cmake -DCMAKE_INSTALL_PREFIX=/usr \
            -DCMAKE_INSTALL_LIBDIR=/usr/lib \
            -DCMAKE_PREFIX_PATH=/opt/vero2 \
            -DCMAKE_INCLUDE_PATH=/opt/vero2/include \
            -DCMAKE_LIBRARY_PATH=/usr/osmc/lib \
            -DOPENGLES_gl_LIBRARY=/opt/vero2/lib \
            -DENABLE_AML=ON \
            -DASS_INCLUDE_DIR=/usr/osmc/lib \
            -DAML_INCLUDE_DIR=/opt/vero2/include \
            -DSHAIRPLAY_INCLUDE_DIR=/usr/osmc/include/shairplay/ \
            -DENABLE_OPENGLES=ON \
            -DENABLE_OPENGL=OFF \
            -DENABLE_OPTICAL=1 \
            -DENABLE_DVDCSS=1 \
            -DWITH_ARCH=arm \
            -DWITH_CPU="cortex-a5" \
            -DCORE_PLATFORM_NAME=aml \
            -DCORE_SYSTEM_NAME=linux \
            -DENABLE_APP_AUTONAME=OFF \
            -DENABLE_INTERNAL_FMT=ON \
            -DENABLE_INTERNAL_FLATBUFFERS=ON \
            -DENABLE_MDNS=OFF \
            -DENABLE_BLUETOOTH=OFF \
            -DENABLE_PULSEAUDIO=OFF \
            -DENABLE_LCMS2=OFF \
            -DENABLE_SNDIO=OFF \
            -DENABLE_MARIADBCLIENT=OFF \
        .
	fi
        if [ "$1" == "vero3" ]; then
        LIBRARY_PATH+="/opt/vero3/lib" && \
        COMPFLAGS="-I/opt/vero3/include -Wl,-rpath=/usr/osmc/lib -L/usr/osmc/lib " && \
        export CFLAGS+=${COMPFLAGS} && \
        export CXXFLAGS+=${COMPFLAGS} && \
        export CPPFLAGS+=${COMPFLAGS} && \
        export LDFLAGS="-L/opt/vero3/lib" && \
        cmake -DCMAKE_INSTALL_PREFIX=/usr \
            -DCMAKE_INSTALL_LIBDIR=/usr/lib \
            -DCMAKE_PREFIX_PATH=/opt/vero3 \
            -DCMAKE_INCLUDE_PATH=/opt/vero3/include \
            -DCMAKE_LIBRARY_PATH=/usr/osmc/lib \
            -DOPENGLES_gl_LIBRARY=/opt/vero3/lib \
            -DENABLE_AML=ON \
            -DASS_INCLUDE_DIR=/usr/osmc/lib \
            -DAML_INCLUDE_DIR=/opt/vero3/include \
            -DSHAIRPLAY_INCLUDE_DIR=/usr/osmc/include/shairplay/ \
            -DENABLE_OPENGLES=ON \
            -DENABLE_OPENGL=OFF \
            -DENABLE_OPTICAL=1 \
            -DENABLE_DVDCSS=1 \
            -DWITH_ARCH=arm \
            -DWITH_CPU="cortex-a53" \
            -DCORE_PLATFORM_NAME=aml \
            -DCORE_SYSTEM_NAME=linux \
            -DENABLE_APP_AUTONAME=OFF \
            -DENABLE_INTERNAL_FMT=ON \
            -DENABLE_INTERNAL_FLATBUFFERS=ON \
	    -DENABLE_MDNS=OFF \
	    -DENABLE_BLUETOOTH=OFF \
	    -DENABLE_PULSEAUDIO=OFF \
	    -DENABLE_LCMS2=OFF \
	    -DENABLE_SNDIO=OFF \
	    -DENABLE_MARIADBCLIENT=OFF \
        .
        fi
	if [ $? != 0 ]; then echo -e "Configure failed!" && exit 1; fi
	$BUILD
	if [ $? != 0 ]; then echo -e "Build failed!" && exit 1; fi
	make install DESTDIR=${out}
	pushd cmake/addons/
	mkdir build
	cd build
        #ADDONS_AUDIO_DECODERS="audiodecoder.modplug audiodecoder.nosefart audiodecoder.sidplay audiodecoder.snesapu"
        ADDONS_AUDIO_ENCODERS="audioencoder.flac audioencoder.lame audioencoder.vorbis audioencoder.wav"
        ADDONS_INPUTSTREAM="inputstream.mpd inputstream.adaptive inputstream.rtmp"
	ADDONS_PERIPHERAL="peripheral.xarcade peripheral.joystick imagedecoder.raw peripheral.steamcontroller"
	ADDONS_PVR="pvr.sledovanitv.cz pvr.argustv pvr.mythtv pvr.hts pvr.pctv pvr.stalker pvr.filmon pvr.octonet pvr.zattoo pvr.vbox pvr.wmc pvr.nextpvr pvr.njoy pvr.teleboy pvr.vdr.vnsi pvr.vuplus pvr.dvbviewer pvr.dvblink pvr.hdhomerun pvr.iptvsimple pvr.demo pvr.mediaportal.tvserver pvr.waipu"
	#ADDONS_SCREENSAVERS="screensaver.biogenesis screensaver.greynetic screensaver.matrixtrails screensaver.pingpong screensaver.pyro screensaver.stars screensaver.shadertoy"
	#ADDONS_SCREENSAVERS="screensaver.shadertoy screensaver.spectrum screensaver.waveform"
	ADDONS_VFS="vfs.libarchive vfs.rar vfs.sftp"
        #ADDONS_VISUALIZATIONS="visualization.fishbmc visualization.goom visualization.projectm visualization.shadertoy visualization.spectrum visualization.vsxu visualization.waveform"
	ADDONS_GAME="game.libretro game.libretro.2048 game.libretro.beetle-gba game.libretro.fceumm game.libretro.gw game.libretro.beetle-pce-fast game.libretro.bnes game.libretro.nestopia game.libretro.bsnes-mercury-balanced game.libretro.mame2000 game.libretro.mame2003 game.libretro.bluemsx game.libretro.vecx game.libretro.mame2003_plus game.libretro.gambatte game.libretro.beetle-bsnes game.libretro.meteor game.libretro.mgba game.libretro.vba-next game.libretro.quicknes game.libretro.scummvm game.libretro.snes9x game.libretro.stella game.libretro.dosbox game.libretro.vbam game.libretro.genplus"
	if [ "$1" == "rbp1" ] || [ "$1" == "rbp2" ]
	then
	    ADDONS_TO_BUILD="${ADDONS_AUDIO_ENCODERS} ${ADDONS_INPUTSTREAM} ${ADDONS_PERIPHERAL} ${ADDONS_PVR} ${ADDONS_SCREENSAVERS} ${ADDONS_VFS} ${ADDONS_VISUALIZATIONS} ${ADDONS_GAME}"
	    PLATFORM="-DCMAKE_INCLUDE_PATH=/opt/vc/include:/opt/vc/include/interface:/opt/vc/include/interface/vcos/pthreads:/opt/vc/include/interface/vmcs_host/linux -DCMAKE_LIBRARY_PATH=/opt/vc/lib"
	fi
	if [ "$1" == "vero2" ]
	then
	   ADDONS_TO_BUILD="${ADDONS_AUDIO_ENCODERS} ${ADDONS_INPUTSTREAM} ${ADDONS_PERIPHERAL} ${ADDONS_PVR} ${ADDONS_SCREENSAVERS} ${ADDONS_VFS} ${ADDONS_VISUALIZATIONS} ${ADDONS_GAME}"
	   PLATFORM="-DCMAKE_INCLUDE_PATH=/opt/vero2/include -DCMAKE_LIBRARY_PATH=/opt/vero2/lib"
	fi
	if [ "$1" == "vero3" ]
	then
	   ADDONS_TO_BUILD="${ADDONS_AUDIO_ENCODERS} ${ADDONS_INPUTSTREAM} ${ADDONS_PERIPHERAL} ${ADDONS_PVR} ${ADDONS_SCREENSAVERS} ${ADDONS_VFS} ${ADDONS_VISUALIZATIONS} ${ADDONS_GAME}"
	   PLATFORM="-DCMAKE_INCLUDE_PATH=/opt/vero3/include -DCMAKE_LIBRARY_PATH=/opt/vero3/lib"
	fi
	if [ "$1" == "pc" ]
	then
           ADDONS_TO_BUILD="${ADDONS_INPUTSTREAM} ${ADDONS_PERIPHERAL} ${ADDONS_PVR}"
           PLATFORM=""
	fi
        cmake -DOVERRIDE_PATHS=1 -DCMAKE_INSTALL_PREFIX=${out}/usr/ -DBUILD_DIR=$(pwd) -DBUILD_SHARED_LIBS=OFF -DADDONS_TO_BUILD="${ADDONS_TO_BUILD}" "$PLATFORM" ../
	if [ $? != 0 ]; then echo "Configuring binary addons failed" && exit 1; fi
	cd ../
	$BUILD kodiplatform_DIR=$(pwd) CMAKE_PREFIX_PATH=/usr/osmc -C build/
	if [ $? != 0 ]; then echo "Building binary addons failed" && exit 1; fi
	popd
        # Languages
        mkdir languages/
        pushd languages
        if [ "$API_VERSION" = "18" ]; then api_name="leia"; fi
	if [ "$API_VERSION" = "19" ]; then api_name="tbc"; fi
	base_url="http://mirror.ox.ac.uk/sites/xbmc.org/addons/${api_name}"
	handle_dep "wget" # We do not usually use wget in the build environment
        languages=$(wget ${base_url} -O- | grep resource.language. | sed -e 's/<a/\n<a/g' | sed -e 's/<a .*href=['"'"'"]//' -e 's/["'"'"'].*$//' -e '/^$/ d' | sed '/tr/d' | sed 's/resource.language.//' | tr -d / | grep -v 'img src')
        if [ $? != 0 ]; then echo "Can't get list of languages" && exit 1; fi
        for language in ${languages}
        do
            echo "Downloading language file for ${language}"
            language_file=$(wget ${base_url}/resource.language.${language} -O- | grep -o '<a .*href=.*>' | sed -e 's/<a/\n<a/g' | sed -e 's/<a .*href=['"'"'"]//' -e 's/["'"'"'].*$//' -e '/^$/ d' | grep 'resource.language' | sort --version-sort | tail -n 1)
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
	COMMON_DEPENDS="niceprioritypolicy-osmc, mediacenter-send-osmc, libssh-4, libavahi-client3, python, python-imaging, python-unidecode, libsmbclient, libjpeg62-turbo, libsqlite3-0, libtinyxml2.6.2v5, libmad0, libmicrohttpd12, libyajl2, libmariadbclient18, libasound2, libxml2, liblzo2-2, libxslt1.1, libpng16-16, libsamplerate0, libtag1v5-vanilla, libfribidi0, libgif7, libcdio13, libpcrecpp0v5, libfreetype6, libvorbis0a, libvorbisenc2, libcurl3, libssl1.1, libplist3, avahi-daemon, policykit-1, mediacenter-addon-osmc (>= 3.0.39), mediacenter-skin-osmc, libcrossguid0, libcap2-bin, libfstrcmp0, libxkbcommon0, libinput10, xz-utils, libiso9660-8, libnss3, libnspr4"
	test "$1" == pc && echo "Depends: ${COMMON_DEPENDS}, amd64-libnfs-osmc, amd64-librtmp-osmc, amd64-libshairplay-osmc, amd64-libbluray-osmc, amd64-libsqlite-osmc, libxrandr2, libglew1.10, libglu1-mesa, xserver-xorg-core, xserver-xorg, xinit, xfonts-base, x11-xserver-utils, xauth, alsa-utils, xserver-xorg-video-intel, amd64-libass-osmc" >> files/DEBIAN/control
	test "$1" == rbp1 && echo "Depends: ${COMMON_DEPENDS}, rbp1-libcec-osmc, armv6l-libnfs-osmc, armv6l-librtmp-osmc, armv6l-libshairplay-osmc, armv6l-libbluray-osmc, armv6l-libsqlite-osmc, rbp-userland-osmc, armv6l-splash-osmc, armv6l-libass-osmc" >> files/DEBIAN/control
	test "$1" == rbp2 && echo "Depends: ${COMMON_DEPENDS}, rbp2-libcec-osmc, armv7-libnfs-osmc, armv7-librtmp-osmc, armv7-libshairplay-osmc, armv7-libbluray-osmc, armv7-libsqlite-osmc, rbp-userland-osmc, armv7-splash-osmc, armv7-libass-osmc" >> files/DEBIAN/control
	test "$1" == vero2 && echo "Depends: ${COMMON_DEPENDS}, vero2-libcec-osmc, armv7-libnfs-osmc, armv7-librtmp-osmc, armv7-libshairplay-osmc, armv7-libbluray-osmc, armv7-libsqlite-osmc, vero2-userland-osmc, armv7-splash-osmc, vero2-libamcodec-osmc, armv7-libass-osmc" >> files/DEBIAN/control
	test "$1" == vero3 && echo "Depends: ${COMMON_DEPENDS}, vero3-libcec-osmc, armv7-libnfs-osmc, armv7-librtmp-osmc, armv7-libshairplay-osmc, armv7-libbluray-osmc, armv7-libsqlite-osmc, vero3-userland-osmc, armv7-splash-osmc, vero3-libamcodec-osmc, armv7-libass-osmc" >> files/DEBIAN/control
	cp patches/${1}-watchdog ${out}/usr/bin/mediacenter
	cp patches/${1}-advancedsettings.xml ${out}/usr/share/kodi/system/advancedsettings.xml
	chmod +x ${out}/usr/bin/mediacenter
	fix_arch_ctl "files/DEBIAN/control"
	fix_arch_ctl "files-debug/DEBIAN/control"
	dpkg_build files/ ${1}-mediacenter-osmc.deb
	dpkg_build files-debug/ ${1}-mediacenter-debug-osmc.deb
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
