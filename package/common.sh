# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

function fix_arch_ctl()
{
	sed '/Architecture/d' -i $1
	test $(arch)x == i686x && echo "Architecture: i386" >> $1
	test $(arch)x == armv7lx && echo "Architecture: armhf" >> $1
	test $(arch)x == x86_64x && echo "Architecture: amd64" >> $1
}

function strip_files()
{
	echo -e "Stripping binaries"
	strip "${1}/usr/lib/*.so.*" > /dev/null 2>&1
	strip "${1}/usr/lib/*.a" > /dev/null 2>&1
	strip "${1}/usr/bin/*" >/dev/null 2>&1
}

function strip_libs()
{
	echo -e "Stripping libaries"
	strip "*.so.*" > /dev/null 2>&1
	strip "*.a" > /dev/null 2>&1
}

function pull_source()
{
	if [ -d ${2} ]; then echo "Cleaning old source" && rm -rf ${2}; fi

	if [[ $1 =~ \.zip$ ]]
	then
	echo -e "Detected ZIP source"
	mkdir ${2}
	wget ${1} -O source.zip
	if [ $? != 0 ]; then echo "Downloading zip failed" && exit 1; fi
	unzip source.zip -d ${2}
	rm source.zip
	return
	fi

	if [[ $1 =~ \.tar$ || $1 =~ \.tgz$ || $1 =~ \.tar\.gz$ || $1 =~ \.tar\.bz2$ ]]
	then
	echo -e "Detected tarball source"
	mkdir ${2}
	wget ${1} -O source.tar
	if [ $? != 0 ]; then echo "Downloading tarball failed" && exit 1; fi
	tar -xvf source.tar -C ${2}
	rm source.tar
	return
	fi

	if [[ $1 =~ git ]]
	then
	echo -e "Detected Git source"
	git clone ${1} ${2}
	if [ $? != 0 ]; then echo "Source checkout failed" && exit 1; fi
	return
	fi

	echo -e "No file type match found for URL"
}

cores=$(if [ ! -f /proc/cpuinfo ]; then mount -t proc proc /proc; fi; cat /proc/cpuinfo | grep processor | wc -l)
export BUILD="make -j${cores}"

export -f fix_arch_ctl
export -f strip_files
export -f strip_libs
