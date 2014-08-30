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
	if [ -d ${2}/src ]; then echo "Cleaning old source" && rm -rf ${2}/src; fi
	echo $1 | grep -q git
	if [ $? == 0 ]
	then
	echo -e "Detected Git source"
	git clone ${1} ${2}/src
	if [ $? != 0 ]; then echo "Source checkout failed" && exit 1; fi
	fi	
}

cores=$(if [ ! -f /proc/cpuinfo ]; then mount -t proc proc /proc; fi; cat /proc/cpuinfo | grep processor | wc -l)
export BUILD="make -j${cores}"

export -f fix_arch_ctl
export -f strip_files
export -f strip_libs
