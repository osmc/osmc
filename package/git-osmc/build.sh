# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building git for QEMU ARM"
make clean
out=$(pwd)/files
sed '/Package/d' -i files/DEBIAN/control
test $1 == rbp && echo "Package: rbp-git-osmc" >> files/DEBIAN/control
cd src
make clean
./configure --prefix=/usr --disable-pthreads
# Hack QEMU
PATH="/tmp:"${PATH}
ln -s /bin/true /tmp/msgfmt
$BUILD NO_GETTEXT=YesPlease
if [ $? != 0 ]; then echo "Error occured during build" && rm /tmp/msgfmt && exit 1; fi
rm /tmp/msgfmt
make install DESTDIR=${out}
cd ../
dpkg -b files/ git-osmc.deb
