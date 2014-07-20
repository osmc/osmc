# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

function fix_arch_ctl()
{
	sed '/Architecture/d' -i $1
	test $(arch)x == i686x && echo "Architecture: i386" >> $1
	test $(arch)x == armv7lx && echo "Architecture: armhf" >> $1
	test $(arch)x == x86_x64x && echo "Architecture: amd64" >> $1
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

cores=$(cat /proc/cpuinfo | grep processor | wc -l)
export BUILD="make -j${cores}"

export -f fix_arch_ctl
export -f strip_files
export -f strip_libs
