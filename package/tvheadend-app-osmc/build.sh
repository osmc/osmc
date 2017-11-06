# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh
VERSION="v4.2.2"
pull_source "https://github.com/tvheadend/tvheadend/archive/${VERSION}.tar.gz" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "tvheadend-app-osmc"
build_return=$?
if [ $build_return == 99 ]
then
	echo -e "Building TVHeadend"
	out=$(pwd)/files
	sed '/Package/d' -i files/DEBIAN/control
	rm -f files/etc/osmc/apps.d/*tvheadend-app-osmc
	update_sources
	handle_dep "pkg-config"
	handle_dep "libssl1.0-dev"
	handle_dep "libavahi-client-dev"
	handle_dep "libcurl3"
	handle_dep "libcurl4-openssl-dev"
	handle_dep "ca-certificates"
	handle_dep "zlib1g-dev"
	handle_dep "liburiparser-dev"
	handle_dep "python-minimal"
	handle_dep "libhdhomerun-dev"
	handle_dep "dvb-tools"
	handle_dep "libdvbv5-0"
	handle_dep "bzip2"
	handle_dep "gzip"
	handle_dep "libsystemd-daemon-dev"
	handle_dep "liburiparser-dev"
	handle_dep "gettext"
	handle_dep "wget"
	handle_dep "libdvbcsa-dev"
	mkdir -p files/etc/osmc/apps.d
	echo "Package: ${1}-tvheadend-app-osmc" >> files/DEBIAN/control && APP_FILE="files/etc/osmc/apps.d/${1}-tvheadend-app-osmc"
    	echo -e "TVHeadend Server\ntvheadend.service" > $APP_FILE
	pushd src/tvheadend*
	install_patch "../../patches" "all"
	./configure --prefix=/usr --enable-hdhomerun_client --disable-hdhomerun_static --disable-ffmpeg_static --disable-libx264 --disable_libx264_static --disable-libx265 --disable-libx265_static --disable-libvpx --disable-libvpx_static --disable-libtheora --disable-libtheorsa_static --disable-libvorbis --disable-libvorbis_static --disable-libfdkaac --disable-libfdkaac_static --disable-nvenc --disable-libmfx_static --disable-android --enable-libsystemd_daemon --disable-kqueue --enable-dbus_1 --disable-tsdebug --disable-gtimer_check --enable-dvbscan --disable-bundle --disable-tvhcsa --enable-uriparser --disable-epoll --enable-zlib --disable-inotify --enable-dvben50221 --disable-dvbcsa --nowerror --enable-dvbcsa --enable-tvhcsa
	sed -e "s/0.0.0~unknown/${VERSION}~osmc/" -i support/version
	$BUILD
	make install DESTDIR=${out}
	if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
	strip_files "${out}"
	popd
	fix_arch_ctl "files/DEBIAN/control"
	publish_applications_targeted "$(pwd)" "$1" "tvheadend-app-osmc"
	dpkg_build files/ ${1}-tvheadend-app-osmc.deb
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
