# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash
if [ -f initramfs.tar ]; then rm initramfs.tar; fi
if [ -d output ]; then rm -rf output; fi

echo -e "Static build of /init"
pushd init
make clean > /dev/null 2>&1
make
popd
echo -e "Static build of /usr/bin/splash"
#TBC, pull from Debian package
popd
mkdir output
mkdir -p output/usr/bin
pushd output
cp -ar ../init/init .
rm -rf proc sys lib tmp var dev etc > /dev/null 2>&1
mkdir proc sys lib tmp var dev etc
chmod 777 tmp
if [ ! -c dev/console ]; then mknod dev/console c 5 1; fi
if [ ! -c dev/ttyS0 ]; then mknod dev/ttyS0 c 204 64; fi
if [ ! -f etc/fstab ]; then echo "none /dev/pts devpts mode=0622 0 0" > etc/fstab; fi
tar -cf ../initramfs.tar *
popd
echo Built initramfs.tar
