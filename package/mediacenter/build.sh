# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building XBMC"
out=$(pwd)/files
if [ -d files/usr ]; then rm -rf files/usr; fi
sed '/Package/d' -i files/DEBIAN/control
sed /'Depends/d' -i files/DEBIAN/control
test "$1" == atv && echo "Package: atv-osmc-mediacenter" >> files/DEBIAN/control
test "$1" == rbp && echo "Package: rbp-osmc-mediacenter" >> files/DEBIAN/control
# Check out XBMC source
XBMC_SRC="https://github.com/xbmc/xbmc"
git clone ${XBMC_SRC} src/
if [ $? != 0 ]; then echo -e "Checkout failed" && exit 1; fi
cd src
BRANCH="Gotham"
git checkout "${BRANCH}"
# Patch generic
sh ../patch-generic.sh
# Patch platform
sh ../patch-"${1}".sh
# Build
./bootstrap
test "$1" == atv && ./configure --prefix=/usr
if [ $? != 0 ]; then echo -e "Configure failed!" && exit 1; fi
make -j4
if [ $? != 0 ]; then echo -e "Build failed!" && exit 1; fi
make install DESTDIR=${out}
# PVR build
#tbc
# Set deps:
test "$1" == atv && echo "Depends: libssh-4 libavahi-client3 python libsmbclient libpulse0 libtiff5 libjpeg8 libsqlite3-0 libflac8 libtinyxml2.6.2 libmicrohttpd10 libjasper1 libxrandr2 libyajl2 libmysqlclient18 libasound2 libxml2 libxslt1.1 libpng12-0 libsamplerate0 libtag1-vanilla libsdl-image1.2 libglew1.10 libfribidi0 liblzo2-2 libcdio13 libpcrecpp0" >> files/DEBIAN/control
fix_arch_ctl "files/DEBIAN/control"
dpkg -b files/ osmc-mediacenter.deb
