# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building XBMC"
out=$(pwd)/files
if [ -d files/usr ]; then rm -rf files/usr; fi
if [ -d src-a/ ]; then rm -rf src-a; fi
if [ -d src-b/ ]; then rm -rf src-b; fi
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
sh ../patch-generic.sh
sh ../patch-"${1}".sh
./bootstrap
test "$1" == atv && CXXFLAGS="-I/usr/include/afpfs-ng" ./configure --prefix=/usr
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
test "$1" == atv && echo "Depends: libssh-4,libavahi-client3,python,libsmbclient,libpulse0,libtiff5,libjpeg8,libsqlite3-0,libflac8,libtinyxml2.6.2,libmicrohttpd10,libjasper1,libxrandr2,libyajl2,libmysqlclient18,libasound2,libxml2,libxslt1.1,libpng12-0,libsamplerate0,libtag1-vanilla,libsdl-image1.2,libglew1.10,libfribidi0,liblzo2-2,libcdio13,libpcrecpp0,libfreetype6,libglu1-mesa,libcrystalhd3,firmware-crystalhd" >> files/DEBIAN/control
fix_arch_ctl "files/DEBIAN/control"
dpkg -b files/ osmc-mediacenter.deb
