# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

function create_systemd_script()
{
	mkdir -p ${2}/lib/systemd/system
	if [ "$1" == "rbp" ]
	then
		echo "[Unit]
Description = media center application
After = remote-fs.target

[Service]
User = osmc
Group = osmc
Type = simple
Environment=LD_PRELOAD=/usr/lib/libarmmem.so
ExecStart = /usr/lib/xbmc/xbmc.bin --standalone -fs
Restart = on-abort

[Install]
WantedBy = multi-user.target" > ${2}/lib/systemd/system/mediacenter.service
fi
chmod +x ${2}/lib/systemd/system/mediacenter.service
}

echo -e "Building XBMC"
out=$(pwd)/files
make clean
sed '/Package/d' -i files/DEBIAN/control
sed /'Depends/d' -i files/DEBIAN/control
test "$1" == atv && echo "Package: atv-osmc-mediacenter" >> files/DEBIAN/control
test "$1" == rbp && echo "Package: rbp-osmc-mediacenter" >> files/DEBIAN/control
XBMC_SRC="https://github.com/xbmc/xbmc"
XBMC_BRANCH="Gotham"
TAG="13.1-Gotham"
git clone "${XBMC_SRC}" -b "${XBMC_BRANCH}" src-a/
if [ $? != 0 ]; then echo -e "Checkout failed" && exit 1; fi
cd src-a
git checkout "${TAG}"
make clean >/dev/null 2>&1
sh ../patch.sh gen
sh ../patch.sh "${1}"
./bootstrap
test "$1" == atv && CXXFLAGS="-I/usr/include/afpfs-ng" ./configure \
--prefix=/usr
test "$1" == rbp && LIBRARY_PATH+=/opt/vc/lib CXXFLAGS="-I/usr/include/afpfs-ng -I/opt/vc/include -I/opt/vc/include/interface -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host/linux -L/opt/vc/lib" ./configure \
	--prefix=/usr \
 	--enable-gles --disable-sdl --disable-x11 --disable-xrandr --disable-openmax \
	--enable-optical-drive --enable-libbluray --enable-dvdcss --disable-joystick --disable-debug \
 	--disable-crystalhd --disable-vtbdecoder --disable-vaapi --disable-vdpau \
 	--disable-pulse --disable-projectm --with-platform=raspberry-pi --enable-optimizations \
 	--enable-libcec --enable-player=omxplayer
if [ $? != 0 ]; then echo -e "Configure failed!" && exit 1; fi
make
if [ $? != 0 ]; then echo -e "Build failed!" && exit 1; fi
make install DESTDIR=${out}
cd ../
PVR_SRC="https://github.com/opdenkamp/xbmc-pvr-addons"
PVR_BRANCH="gotham"
PVR_TAG="13.0-Gotham"
git clone "${PVR_SRC}" -b "${PVR_BRANCH}" src-b/
cd src-b
git checkout "${TAG}"
make clean >/dev/null 2>&1
sh ../patch-pvr-generic.sh
./bootstrap
./configure --prefix=/usr --enable-addons-with-dependencies
if [ $? != 0 ]; then echo -e "Configure failed!" && exit 1; fi
make
if [ $? != 0 ]; then echo -e "Build failed!" && exit 1; fi
make install DESTDIR=${out}
cd ../
rm -rf ${out}/usr/share/xbmc/addons/service.xbmc.versioncheck
strip ${out}/usr/lib/xbmc/xbmc.bin
strip ${out}/usr/lib/xbmc/addons/*/*.so
strip ${out}/usr/lib/xbmc/addons/pvr.*/*.pvr
test "$1" == atv && echo "Depends: niceprioritypolicy-osmc,mediacenter-send-osmc,libssh-4,libavahi-client3,python,libsmbclient,libpulse0,libtiff5,libjpeg8,libsqlite3-0,libflac8,libtinyxml2.6.2,libogg0,libmad0,libmicrohttpd10,libjasper1,libxrandr2,libyajl2,libmysqlclient18,libasound2,libxml2,libxslt1.1,libpng12-0,libsamplerate0,libtag1-vanilla,libsdl-image1.2,libglew1.10,libfribidi0,liblzo2-2,libcdio13,libpcrecpp0,libfreetype6,libvorbisenc2,libcurl3-gnutls,libglu1-mesa,libcec-osmc,libshairplay-osmc,libnfs-osmc,libafpclient-osmc,librtmp-osmc,libcrystalhd3,firmware-crystalhd" >> files/DEBIAN/control
test "$1" == rbp && echo "Depends: niceprioritypolicy-osmc,mediacenter-send-osmc,libssh-4,libavahi-client3,python,libsmbclient,libtiff5,libjpeg8,libsqlite3-0,libflac8,libtinyxml2.6.2,libogg0,libmad0,libmicrohttpd10,libjasper1,libyajl2,libmysqlclient18,libasound2,libxml2,libxslt1.1,libpng12-0,libsamplerate0,libtag1-vanilla,libfribidi0,liblzo2-2,libcdio13,libpcrecpp0,libfreetype6,libvorbisenc2,libcurl3-gnutls,rbp-libcec-osmc,rbp-libshairplay-osmc,rbp-libnfs-osmc,rbp-libafpclient-osmc,rbp-librtmp-osmc,rpiuserland,rpi-armmem-osmc" >> files/DEBIAN/control
cp ${out}/../splash.png ${out}/usr/share/xbmc/media/Splash.png
test "$1" == rbp && create_systemd_script "rbp" "${out}"
fix_arch_ctl "files/DEBIAN/control"
dpkg -b files/ osmc-mediacenter.deb
