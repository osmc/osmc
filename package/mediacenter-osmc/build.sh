# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

if [ "$1" == "rbp2" ] || [ "$1" == "rbp4" ] || [ "$1" == "pc" ] || [ "$1" == "vero3" ]
then
pull_source "https://github.com/xbmc/xbmc/archive/2382f8e725d1865a22b6ef7a24e6b64f078c2e01.tar.gz" "$(pwd)/src"
API_VERSION="19"
else
pull_source "https://github.com/xbmc/xbmc/archive/master.tar.gz" "$(pwd)/kodi"
API_VERSION="20"
fi
if [ $? != 0 ]; then echo -e "Error fetching Kodi source" && exit 1; fi
# Build in native environment
BUILD_OPTS=$BUILD_OPTION_DEFAULTS
BUILD_OPTS=$(($BUILD_OPTS - $BUILD_OPTION_USE_CCACHE))
if [ "$1" == "rbp2" ] || [ "$1" == "rbp4" ] || [ "$1" == "vero3" ]
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
	handle_dep "libgtest-dev"
	handle_dep "libinput-dev"
	handle_dep "libgif-dev"
	handle_dep "libiso9660-dev"
	handle_dep "libjpeg62-turbo-dev"
	handle_dep "liblzo2-dev"
	handle_dep "libmad0-dev"
	handle_dep "libmicrohttpd-dev"
	handle_dep "libmodplug-dev"
	handle_dep "libmariadbd-dev"
	handle_dep "libpcre3-dev"
	handle_dep "libplist-dev"
	handle_dep "libpng-dev"
	handle_dep "libsmbclient-dev"
	handle_dep "libssh-dev"
	handle_dep "libavahi-client-dev"
	handle_dep "libssl-dev" # We need this as well as libcurl4-openssl-dev because openssl libs are only suggestions
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
	handle_dep "python3-dev"
	handle_dep "python3-pil"
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
        handle_dep "libgcrypt20-dev"
        handle_dep "libnfs-dev"
        handle_dep "libass-dev"
	handle_dep "libunistring-dev"
	handle_dep "xmlstarlet"
	handle_dep "meson"
	handle_dep "libflatbuffers-dev"
	handle_dep "libspdlog-dev"
	handle_dep "libfmt-dev"
	if [ "$1" == "rbp2" ] || [ "$1" == "rbp4" ]
	then
		handle_dep "rbp2-libcec-dev-osmc"
		handle_dep "armv7-libshairplay-dev-osmc"
		handle_dep "armv7-librtmp-dev-osmc"
		handle_dep "armv7-libplatform-dev-osmc"
		handle_dep "armv7-libbluray-dev-osmc"
		handle_dep "armv7-libsqlite-dev-osmc"
	fi
	if [ "$1" == "vero3" ]
	then
		handle_dep "vero3-libcec-dev-osmc"
		handle_dep "vero3-userland-dev-osmc"
		handle_dep "armv7-libshairplay-dev-osmc"
                handle_dep "armv7-librtmp-dev-osmc"
                handle_dep "armv7-libplatform-dev-osmc"
                handle_dep "armv7-libbluray-dev-osmc"
                handle_dep "armv7-libsqlite-dev-osmc"
		handle_dep "libamcodec-dev-osmc"
	fi
	if [ "$1" == "pc" ]
	then
		handle_dep "amd64-libshairplay-dev-osmc"
		handle_dep "amd64-librtmp-dev-osmc"
		handle_dep "amd64-libplatform-dev-osmc"
		handle_dep "amd64-libbluray-dev-osmc"
		handle_dep "amd64-libsqlite-dev-osmc"
		handle_dep "xserver-xorg-dev"
		handle_dep "libxrandr-dev"
		handle_dep "x11proto-randr-dev"
		handle_dep "libegl1-mesa-dev"
		handle_dep "libglew-dev"
	fi
	if [ "$1" == "rbp2" ] || [ "$1" == "rbp4" ]
	then
		handle_dep "libdrm-dev"
		handle_dep "rbp2-mesa-dev-osmc"
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
	cp -ar ../../patches/resource.language.*/ addons/
	test "$1" == pc && install_patch "../../patches" "pc"
	if [ "$1" == "rbp2" ] || [ "$1" == "rbp4" ]
	then
		install_patch "../../patches" "rbp"
	fi
	if [ "$1" == "rbp2" ] || [ "$1" == "vero3" ]; then install_patch "../../patches" "arm"; fi
	test "$1" == vero3 && install_patch "../../patches" "vero3"
	mkdir kodi-build
	pushd kodi-build
	# Raspberry Pi Configuration
	if [ "$1" == "rbp2" ]
	then
		COMPFLAGS="-march=armv7-a -mfloat-abi=hard -O3 -mfpu=neon-vfpv4 -fomit-frame-pointer "
	fi
	if [ "$1" == "rbp4" ]
	then
		COMPFLAGS="-march=armv8-a -O3 -mfpu=neon-fp-armv8 -fomit-frame-pointer -mvectorize-with-neon-quad "
	fi
	if [ "$1" == "vero3" ]
	then
		COMPFLAGS="-march=armv7-a -mfloat-abi=hard -O3 -marm -mfpu=neon -fomit-frame-pointer "
	fi
	if [ "$1" == "rbp2" ] || [ "$1" == "rbp4" ]; then
	# Check if we have headers for kernel 5.x
	dpkg -l | grep rbp2-headers-sanitised | grep 5
	if [ $? -eq 0 ]
	then
	    # We have some installed, let's find them
	    headers_version=$(dpkg -l | grep rbp2-headers-sanitised | grep 5 | awk '{print $2'})
	else
	    headers_version=$(apt-cache search rbp2-headers-sanitised | grep 5 | tail -n 1 | awk {'print $1'})
	    handle_dep "${headers_version}"
	fi
        COMPFLAGS+="-I/usr/osmc/include/ -I/usr/osmc/include/EGL -I/usr/src/${headers_version}/include -L/usr/osmc/lib -Wl,-rpath=/usr/osmc/lib" && \
        export CFLAGS="${COMPFLAGS} ${CFLAGS}" && \
        export CXXFLAGS="${COMPFLAGS} ${CFLAGS}" && \
        export CPPFLAGS="${COMPFLAGS} ${CFLAGS}" && \
	cmake -DCMAKE_INSTALL_PREFIX=/usr \
            -DCMAKE_INSTALL_LIBDIR=/usr/lib \
            -DCMAKE_INCLUDE_PATH=/usr/osmc/include \
            -DCMAKE_LIBRARY_PATH=/usr/osmc/lib \
            -DASS_INCLUDE_DIR=/usr/osmc/lib \
            -DSHAIRPLAY_INCLUDE_DIR=/usr/osmc/include/shairplay/ \
            -DENABLE_OPENGLES=ON \
            -DENABLE_OPENGL=OFF \
            -DENABLE_OPTICAL=1 \
            -DENABLE_DVDCSS=1 \
            -DCORE_SYSTEM_NAME=linux \
            -DCORE_PLATFORM_NAME=gbm \
            -DAPP_RENDER_SYSTEM=gles \
            -DWITH_ARCH=arm \
            -DENABLE_APP_AUTONAME=OFF \
            -DENABLE_INTERNAL_FMT=OFF \
            -DENABLE_INTERNAL_FLATBUFFERS=OFF \
            -DENABLE_INTERNAL_SPDLOG=OFF \
            -DENABLE_INTERNAL_UDFREAD=ON \
            -DENABLE_MDNS=OFF \
            -DENABLE_BLUETOOTH=OFF \
            -DENABLE_PULSEAUDIO=OFF \
            -DENABLE_LCMS2=OFF \
            -DENABLE_SNDIO=OFF \
            -DENABLE_MARIADBCLIENT=ON \
	    -DENABLE_VAAPI=OFF \
            -DENABLE_VDPAU=OFF \
            -DENABLE_INTERNAL_DAV1D=ON \
        ../
	fi
        if [ "$1" == "vero3" ]; then
        LIBRARY_PATH+="/opt/vero3/lib" && \
        COMPFLAGS+="-I/opt/vero3/include -L/opt/vero3/lib -L/usr/osmc/lib -Wl,-rpath=/usr/osmc/lib" && \
        export CFLAGS="${COMPFLAGS} ${CFLAGS}" && \
        export CXXFLAGS="${COMPFLAGS} ${CFLAGS}" && \
        export CPPFLAGS="${COMPFLAGS} ${CFLAGS}" && \
        export LDFLAGS="-L/opt/vero3/lib" && \
        cmake -DCMAKE_INSTALL_PREFIX=/usr \
            -DCMAKE_INSTALL_LIBDIR=/usr/lib \
            -DCMAKE_PREFIX_PATH=/opt/vero3 \
            -DCMAKE_INCLUDE_PATH=/opt/vero3/include \
            -DCMAKE_LIBRARY_PATH=/usr/osmc/lib \
            -DOPENGLES_gl_LIBRARY=/opt/vero3/lib \
            -DENABLE_AML=ON \
            -DAPP_RENDER_SYSTEM=gles \
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
            -DENABLE_INTERNAL_FMT=OFF \
            -DENABLE_INTERNAL_FLATBUFFERS=OFF \
            -DENABLE_INTERNAL_SPDLOG=OFF \
            -DENABLE_INTERNAL_UDFREAD=ON \
	    -DENABLE_MDNS=OFF \
	    -DENABLE_BLUETOOTH=OFF \
	    -DENABLE_PULSEAUDIO=OFF \
	    -DENABLE_LCMS2=OFF \
	    -DENABLE_SNDIO=OFF \
            -DENABLE_MARIADBCLIENT=ON \
            -DENABLE_INTERNAL_DAV1D=ON \
	../
        fi
	if [ $? != 0 ]; then echo -e "Configure failed!" && exit 1; fi
	$BUILD
	if [ $? != 0 ]; then echo -e "Build failed!" && exit 1; fi
	make install DESTDIR=${out}
	popd
	KODIDIR=$(pwd)/kodi-build
	pushd cmake/addons/
	mkdir build
	cd build
	# One day we might need to change this in to ADDONS_XYZ_PLAT
        ADDONS_AUDIO_DECODERS="audiodecoder.2sf audiodecoder.asap audiodecoder.dumb audiodecoder.gme audiodecoder.gsf audiodecoder.modplug audiodecoder.ncsf audiodecoder.nosefart audiodecoder.openmpt audiodecoder.organya audiodecoder.qsf audiodecoder.sacd audiodecoder.sidplay audiodecoder.snesapu audiodecoder.ssf audiodecoder.stsound audiodecoder.timidity audiodecoder.upse audiodecoder.vgmstream audiodecoder.wsr audiodecoder.hvl"
        ADDONS_AUDIO_ENCODERS="audioencoder.flac audioencoder.lame audioencoder.vorbis audioencoder.wav"
        ADDONS_INPUTSTREAM="inputstream.adaptive inputstream.rtmp inputstream.ffmpegdirect"
	ADDONS_PERIPHERAL="peripheral.xarcade peripheral.joystick"
	ADDONS_PVR="pvr.argustv pvr.dvblink pvr.dvbviewer pvr.filmon pvr.freebox pvr.hdhomerun pvr.hts pvr.iptvsimple pvr.mediaportal.tvserver pvr.mythtv pvr.nextpvr pvr.njoy pvr.octonet pvr.pctv pvr.sledovanitv.cz pvr.stalker pvr.teleboy pvr.vbox pvr.vdr.vnsi pvr.vuplus pvr.waipu pvr.wmc pvr.zattoo"
	ADDONS_SCREENSAVERS="screensaver.asteroids screensaver.asterwave screensaver.biogenesis screensaver.cpblobs screensaver.greynetic screensaver.matrixtrails screensaver.pingpong screensaver.pyro screensaver.shadertoy"
	ADDONS_VFS="vfs.libarchive vfs.rar vfs.sftp"
        ADDONS_VISUALIZATIONS="visualization.fishbmc visualization.goom visualization.matrix visualization.milkdrop visualization.milkdrop2 visualization.pictureit visualization.shadertoy visualization.spectrum visualization.starburst visualization.waveform"
	ADDONS_GAME="game.libretro game.libretro.2048" # Needs updating from https://github.com/kodi-game/repo-binary-addons
	ADDONS_IMAGE_DECODERS="imagedecoder.heif imagedecoder.mpo imagedecoder.raw"
	ALL="${ADDONS_AUDIO_DECODERS} ${ADDONS_AUDIO_ENCODERS} ${ADDONS_INPUTSTREAM} ${ADDONS_PERIPHERAL} ${ADDONS_PVR} ${ADDONS_SCREENSAVERS} ${ADDONS_VFS} ${ADDONS_VISUALIZATIONS} ${ADDONS_GAME} ${ADDONS_IMAGE_DECODERS}"
	COMPFLAGS+=" -fPIE -fPIC "
	echo "set (CMAKE_C_FLAGS \"${COMPFLAGS}\")" >> ../Toolchain.mk
        echo "set (CMAKE_CXX_FLAGS \"${COMPFLAGS}\")" >> ../Toolchain.mk
        echo "set (CMAKE_LD_FLAGS \"${COMPFLAGS}\")" >> ../Toolchain.mk
	## Don't enable this unless you know what you are doing
        # echo "set(BUILD_SHARED_LIBS 1)" >> ../Toolchain.mk
	##
	if [ "$1" == "rbp2" ] || [ "$1" == "rbp4" ]
	then
	   ADDONS_TO_BUILD="${ALL}"
           echo "set(APP_RENDER_SYSTEM gles)" >> ../Toolchain.mk
           echo "set(OPENGLES_gl_LIBRARY /usr/osmc/lib)" >> ../Toolchain.mk
           echo "set(OPENGLES_INCLUDE_DIR /usr/osmc/include)" >> ../Toolchain.mk
	fi
	if [ "$1" == "vero3" ]
	then
	   ADDONS_TO_BUILD="${ALL}"
           echo "set(APP_RENDER_SYSTEM gles)" >> ../Toolchain.mk
           echo "set(OPENGLES_gl_LIBRARY /opt/vero3/lib)" >> ../Toolchain.mk
           echo "set(OPENGLES_INCLUDE_DIR /opt/vero3/include)" >> ../Toolchain.mk
	fi
	if [ "$1" == "pc" ]
	then
           ADDONS_TO_BUILD="${ADDONS_INPUTSTREAM} ${ADDONS_PERIPHERAL} ${ADDONS_PVR}"
           PLATFORM=""
	fi
        cmake -DOVERRIDE_PATHS=1 -DCMAKE_INSTALL_PREFIX=${out}/usr/ -DBUILD_DIR=$(pwd) -DADDONS_TO_BUILD="${ADDONS_TO_BUILD}" -DCMAKE_TOOLCHAIN_FILE=../Toolchain.mk ../
        if [ $? != 0 ]; then echo "Configuring binary addons failed" && exit 1; fi
        cd ../
	# Update Matrix screensaver logo
	cp ../../../../patches/logo.png visualization.matrix/visualization.matrix/resources/textures/logo.png || true # Just in case user isn't building binaddons
	$BUILD -C build/
	#make -j1 -C build/
	if [ $? != 0 ]; then echo "Building binary addons failed" && exit 1; fi
	popd
        # Languages
        mkdir languages/
        pushd languages
        if [ "$API_VERSION" = "19" ]; then api_name="matrix"; fi
	if [ "$API_VERSION" = "20" ]; then api_name="tbc"; fi
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
	# Prevent language updates
	for language in ${out}/usr/share/kodi/addons/resource.language.*; do xmlstarlet edit --inplace --update '/addon/@version' -v "999.999.999" $language/addon.xml; done
	popd
	popd
	cp -ar patches/resource.language.*/ ${out}/usr/share/kodi/addons/
	rm -rf ${out}/usr/share/kodi/addons/service.*.versioncheck
	mkdir -p files-debug/usr/lib/kodi
	cp -ar ${out}/usr/lib/kodi/kodi.bin files-debug/usr/lib/kodi/kodi.bin
	strip -s ${out}/usr/lib/kodi/kodi.bin
	COMMON_DEPENDS="niceprioritypolicy-osmc, mediacenter-send-osmc, libssh-4, libavahi-client3, python3, python3-pil, python3-unidecode, libpython3.7, libsmbclient, samba-common-bin, libjpeg62-turbo, libsqlite3-0, libtinyxml2.6.2v5, libmad0, libmicrohttpd12, libyajl2, libmariadb3, libasound2, libxml2, liblzo2-2, libxslt1.1, libpng16-16, libsamplerate0, libtag1v5-vanilla, libfribidi0, libgif7, libcdio18, libpcrecpp0v5, libfreetype6, libvorbis0a, libvorbisenc2, libcurl4, libssl1.1, libplist3, avahi-daemon, policykit-1, mediacenter-addon-osmc (>= 3.0.39), mediacenter-skin-osmc, libcrossguid0, libcap2-bin, libfstrcmp0, libxkbcommon0, libinput10, xz-utils, libiso9660-11, libnss3, libnspr4, libnfs12, libass9, libunistring2, libatomic1, libfmt7"
	test "$1" == pc && echo "Depends: ${COMMON_DEPENDS}, libnfs12, amd64-librtmp-osmc, amd64-libshairplay-osmc, amd64-libbluray-osmc, amd64-libsqlite-osmc, libxrandr2, libglew1.10, libglu1-mesa, xserver-xorg-core, xserver-xorg, xinit, xfonts-base, x11-xserver-utils, xauth, alsa-utils, xserver-xorg-video-intel" >> files/DEBIAN/control
	test "$1" == rbp2 && echo "Depends: ${COMMON_DEPENDS}, rbp2-libcec-osmc, armv7-librtmp-osmc, armv7-libshairplay-osmc, armv7-libbluray-osmc, armv7-libsqlite-osmc, rbp-userland-osmc, armv7-splash-osmc, rbp2-mesa-osmc, libdrm2, libglapi-mesa" >> files/DEBIAN/control
	test "$1" == rbp4 && echo "Depends: ${COMMON_DEPENDS}, rbp2-libcec-osmc, armv7-librtmp-osmc, armv7-libshairplay-osmc, armv7-libbluray-osmc, armv7-libsqlite-osmc, rbp-userland-osmc, armv7-splash-osmc, rbp2-mesa-osmc, libdrm2, libglapi-mesa" >> files/DEBIAN/control
	test "$1" == vero3 && echo "Depends: ${COMMON_DEPENDS}, vero3-libcec-osmc, armv7-librtmp-osmc, armv7-libshairplay-osmc, armv7-libbluray-osmc, armv7-libsqlite-osmc, vero3-userland-osmc, armv7-splash-osmc, libamcodec-osmc" >> files/DEBIAN/control
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
