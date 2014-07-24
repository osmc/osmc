# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash
if [ -f initramfs.tar ]; then rm initramfs.tar; fi

BUSYBOX_VERSION="1.21.1"

echo -e "Static build of /init"
pushd init
make clean
make
popd
echo -e "Static build of /usr/bin/splash"
pushd ply-lite
make clean
make
popd
echo -e "Building BusyBox"
pushd busybox-${BUSYBOX_VERSION}
make clean
make -j4
make install
pushd _install
rm -rf proc sys lib tmp var dev etc > /dev/null 2>&1
mkdir proc sys lib tmp var dev etc
chmod 777 tmp
if [ ! -c dev/console ]; then mknod dev/console c 5 1; fi
if [ ! -c dev/ttyS0 ]; then mknod dev/ttyS0 c 204 64; fi
if [ ! -f etc/fstab ]; then echo "none /dev/pts devpts mode=0622 0 0" > etc/fstab; fi
if [ ! -f etc/inittab ]
then
echo "console::sysinit:/etc/init.d/rcS
console::askfirst:/bin/login
::restart:/sbin/init
::ctrlaltdel:/sbin/reboot
::shutdown:/bin/umount -a -r
" > etc/inittab
fi
cp -ar ../../init/init init
cp -ar ../../ply-lite/ply-image usr/bin/splash
tar -cf ../../initramfs.tar *
popd
echo Built initramfs.tar
