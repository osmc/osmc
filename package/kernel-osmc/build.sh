# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

echo -e "Building Linux kernel"
make clean
sed '/Package/d' -i files/DEBIAN/control
sed '/Depends/d' -i files/DEBIAN/control
echo -e "Downloading kernel sources"
handle_dep "kernel-package"
handle_dep "liblz4-tool"
echo "maintainer := Sam G Nazarko
email := email@samnazarko.co.uk
priority := High" >/etc/kernel-pkg.conf
JOBS=$(if [ ! -f /proc/cpuinfo ]; then mount -t proc proc /proc; fi; cat /proc/cpuinfo | grep processor | wc -l && umount /proc/ >/dev/null 2>&1)
test $1 == rbp && VERSION="3.17.4" && REVISION="1" && ARCH = "arm"
if [ -z $VERSION ]; then echo "Don't have a defined kernel version for this target!" && exit 1; fi
echo -e "Downloading kernel sources"
pull_source "https://www.kernel.org/pub/linux/kernel/v3.x/linux-${VERSION}.tar.xz" "${pwd}/src/"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
pushd src
install_patch "../patches" "all"
test "$1" == "rbp" && install_patch "../patches" "rbp"
make-kpkg --stem $1 kernel_image --append-to-version -osmc --jobs $JOBS --revision $REV --arch $ARCH
if [ $? != 0 ]; then echo "Building kernel image package failed" && exit 1; fi
make-kpkg --stem $1 kernel_headers --append-to-version -osmc --jobs $JOBS --revision $REV
if [ $? != 0 ]; then echo "Building kernel headers package failed" && exit 1; fi
popd
echo "Package: kernel-${1}-osmc" >> files/DEBIAN/control && echo "Depends: $1-image-${VERSION}-osmc" >> files-dev/DEBIAN/control
fix_arch_ctl "files/DEBIAN/control"
dpkg -b files/ kernel-{$1}-osmc.deb
echo -e "Build complete"
