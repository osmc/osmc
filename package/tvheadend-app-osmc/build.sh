# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

pull_source "https://github.com/tvheadend/tvheadend" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "tvheadend-app-osmc"
if [ $? == 0 ]
then
	echo -e "Building TVHeadend"
	out=$(pwd)/files
	sed '/Package/d' -i files/DEBIAN/control
	update_sources
	handle_dep "pkg-config"
	handle_dep "libssl-dev"
	handle_dep "git" # for dvbscan info?
	test $1 == gen && echo "Package: tvheadend-app-osmc" >> files/DEBIAN/control
	test $1 == rbp && echo "Package: rbp-tvheadend-app-osmc" >> files/DEBIAN/control
	test $1 == armv7 && echo "Package: armv7-tvheadend-app-osmc" >> files/DEBIAN/control
	pushd src
	git checkout v3.9
	./configure --prefix=/usr
	$BUILD
	make install DESTDIR=${out}
	if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
	strip_files "${out}"
	popd
	fix_arch_ctl "files/DEBIAN/control"
	dpkg -b files/ tvheadend-app-osmc.deb
fi
teardown_env "${1}"
