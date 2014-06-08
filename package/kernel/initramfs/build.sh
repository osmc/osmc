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
mkdir proc sys lib tmp var dev etc
chmod 777 tmp
mknod dev/console c 5 1
mknod dev/ttyS0 c 204 64
echo "none /dev/pts devpts mode=0622 0 0" > etc/fstab
echo "console::sysinit:/etc/init.d/rcS
console::askfirst:/bin/login
::restart:/sbin/init
::ctrlaltdel:/sbin/reboot
::shutdown:/bin/umount -a -r
" > etc/inittab
cp -ar ../../init/init init
cp -ar ../../ply-lite/ply-image usr/bin/splash
tar -cf ../../initramfs.tar *
popd
popd
echo Built initramfs.tar
