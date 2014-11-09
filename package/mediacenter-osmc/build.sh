# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building package Kodi"
out=$(pwd)/files
make clean
sed '/Package/d' -i files/DEBIAN/control
sed /'Depends/d' -i files/DEBIAN/control
test "$1" == atv && echo "Package: atv-mediacenter-osmc" >> files/DEBIAN/control
test "$1" == rbp && echo "Package: rbp-mediacenter-osmc" >> files/DEBIAN/control
KODI_SRC="https://github.com/xbmc/xbmc"
KODI_BRANCH="master"
git clone "${KODI_SRC}" -b "${KODI_BRANCH}" Kodi/
if [ $? != 0 ]; then echo -e "Checkout failed" && exit 1; fi
cd Kodi/
install_patch "../patches" "all"
test "$1" == atv && install_patch "../patches" "atv"
test "$1" == rbp && install_patch "../patches" "rbp" && install_patch "../patches" "lpr"
./bootstrap
# Apple TV configuration
test "$1" == atv && \
CXXFLAGS="-I/usr/include/afpfs-ng" ./configure \
	--prefix=/usr \
	--disable-vtbdecoder \ 
 	--disable-vaapi \ 
 	--disable-vdpau \
 	--disable-pulse \ 
 	--disable-projectm 
# Raspberry Pi Configuration
test "$1" == rbp && \ 
LIBRARY_PATH+=/opt/vc/lib \ 
CXXFLAGS="-I/usr/include/afpfs-ng -I/opt/vc/include -I/opt/vc/include/interface -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host/linux -L/opt/vc/lib" \ 
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
 	--disable-crystalhd \ 
 	--disable-vtbdecoder \ 
 	--disable-vaapi \ 
 	--disable-vdpau \
 	--disable-pulse \ 
 	--disable-projectm \ 
 	--with-platform=raspberry-pi \
 	--enable-optimizations \
 	--enable-libcec \
 	--enable-player=omxplayer
if [ $? != 0 ]; then echo -e "Configure failed!" && exit 1; fi
$BUILD
if [ $? != 0 ]; then echo -e "Build failed!" && exit 1; fi
make install DESTDIR=${out}
cd ../
PVR_SRC="https://github.com/opdenkamp/xbmc-pvr-addons"
PVR_BRANCH="master"
git clone "${PVR_SRC}" -b "${PVR_BRANCH}" Kodi-PVR
cd Kodi-PVR
make clean >/dev/null 2>&1
./bootstrap
./configure --prefix=/usr --enable-addons-with-dependencies
if [ $? != 0 ]; then echo -e "Configure failed!" && exit 1; fi
$BUILD
if [ $? != 0 ]; then echo -e "Build failed!" && exit 1; fi
make install DESTDIR=${out}
cd ../
rm -rf ${out}/usr/share/xbmc/addons/service.kodi.versioncheck
strip ${out}/usr/lib/kodi/kodi.bin
strip ${out}/usr/lib/kodi/addons/*/*.so
strip ${out}/usr/lib/kodi/addons/pvr.*/*.pvr
COMMON_DEPENDS="niceprioritypolicy-osmc, mediacenter-send-osmc, libssh-4, libavahi-client3, python, libsmbclient, libtiff5, libjpeg8, libsqlite3-0, libflac8, libtinyxml2.6.2, libogg0, libmad0, libmicrohttp10, libjasper1, libyajl2, libmysqlclient18, libasound2, libxml2, libxslt1.1, libpng12-0, libsamplerate0, libtag1-vanilla, libfribidi0, libzlk02-2, libcdio13, libpcrecpp0, libfreetype6, libvorbisenc2, libcurl3-gnutls"
X86_DEPENDS="libcec-osmc, libshairplay-osmc, libnfs-osmc, libafpclient-osmc, librtmp-osmc"
test "$1" == atv && echo "Depends: ${COMMON_DEPENDS}, ${X86_DEPENDS}, libpulse0, libxrandr2, libsdl-image1.2, libglew1.10, libglu1-mesa, libcrystalhd3, firmware-crystalhd" >> files/DEBIAN/control
test "$1" == rbp && echo "Depends: ${COMMON_DEPENDS}, rbp-libcec-osmc, rbp-libafpclient-osmc, rbp-librtmp-osmc, rbp-userland-osmc, rbp-armmem-osmc" >> files/DEBIAN/control
chmod +x ${out}/lib/systemd/system/mediacenter.service
cp ${1}-watchdog ${out}/usr/bin/mediacenter
chmod +x ${out}/usr/bin/mediacenter
fix_arch_ctl "files/DEBIAN/control"
dpkg -b files/ mediacenter-osmc.deb
