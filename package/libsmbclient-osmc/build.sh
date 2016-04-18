# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

pull_source "https://download.samba.org/pub/samba/stable/samba-3.6.25.tar.gz" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "libsmbclient-osmc"
build_return=$?
if [ $build_return == 99 ]
then
	echo -e "Building libsmbclient"
	out=$(pwd)/files
	if [ -d files/usr ]; then rm -rf files/usr; fi
	if [ -d files-dev/usr ]; then rm -rf files-dev/usr; fi
	sed '/Package/d' -i files/DEBIAN/control
	sed '/Package/d' -i files-dev/DEBIAN/control
	sed '/Depends/d' -i files-dev/DEBIAN/control
        sed '/Version/d' -i files-dev/DEBIAN/control
        VERSION_DEV=$(grep Version ${out}/DEBIAN/control)
        VERSION_NUM=$(echo $VERSION_DEV | awk {'print $2'})
        echo $VERSION_DEV >> files-dev/DEBIAN/control
        echo "Depends: ${1}-libsmbclient-osmc (=${VERSION_NUM})" >> files-dev/DEBIAN/control
	update_sources
	handle_dep "autoconf"
	handle_dep "automake"
	handle_dep "libtool"
	echo "Package: ${1}-libsmbclient-osmc" >> files/DEBIAN/control && echo "Package: ${1}-libsmbclient-dev-osmc" >> files-dev/DEBIAN/control
	pushd src/samba-*/source3/
	install_patch "../../../patches" "all"
	./autogen.sh
	./configure --prefix=/usr/osmc --without-cluster-support --disable-swat --without-ldap --without-pam \
	--without-pam_smbpass --with-fhs --with-libtalloc=no \
	--with-libtdb=no --without-winbind --disable-cups --without-ads \
	--disable-avahi --disable-fam --without-libaddns --without-libnetapi \
	--without-dnsupdate --without-libsmbsharemodes --with-libiconv
	$BUILD libsmbclient
	make installlibsmbclient DESTDIR=${out}
        if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
        strip_files "${out}"
        popd
        mkdir -p files-dev/usr/osmc
        mv files/usr/osmc/include files-dev/usr/osmc
	fix_arch_ctl "files/DEBIAN/control"
	fix_arch_ctl "files-dev/DEBIAN/control"
	dpkg_build files ${1}-libsmbclient-osmc.deb
	dpkg_build files-dev ${1}-libsmbclient-dev-osmc.deb
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
