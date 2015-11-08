# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

pull_source "http://sqlite.org/2015/sqlite-autoconf-3090200.tar.gz" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "libsqlite-osmc"
build_return=$?
if [ $build_return == 99 ]
then
	echo -e "Building libsqlite"
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
        echo "Depends: ${1}-libsqlite-osmc (=${VERSION_NUM})" >> files-dev/DEBIAN/control
	update_sources
	handle_dep "autoconf"
	handle_dep "libtool"
	echo "Package: ${1}-libsqlite-osmc" >> files/DEBIAN/control && echo "Package: ${1}-libsqlite-dev-osmc" >> files-dev/DEBIAN/control
	pushd src/sqlite*
	install_patch "../../patches" "all"
	./configure --prefix=/usr/osmc --enable-threadsafe --disable-readline
	export CXXFLAGS+="-DSQLITE_ENABLE_COLUMN_METADATA=1"
	export CFLAGS+="-DSQLITE_TEMP_STORE=3 -DSQLITE_DEFAULT_MMAP_SIZE=0x10000000"
	export TCLLIBDIR="/dev/null"
	$BUILD
	make install DESTDIR=${out}
	rm -rf ${out}/usr/bin/
	if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
	strip_files "${out}"
	popd
	mkdir -p files-dev/usr/osmc
	mv files/usr/osmc/include  files-dev/usr/osmc
	fix_arch_ctl "files/DEBIAN/control"
	fix_arch_ctl "files-dev/DEBIAN/control"
	dpkg_build files ${1}-libsqlite-osmc.deb
	dpkg_build files-dev ${1}-libsqlite-dev-osmc.deb
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
