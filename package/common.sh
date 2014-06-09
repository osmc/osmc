# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

function fix_arch_ctl()
{
	sed '/Architecture/d' -i $1
	test $(arch)x == i686x && echo "Architecture: i386" >> $1
	test $(arch)x == armv7lx && echo "Architecture: armhf" >> $1
	test $(arch)x == x86_x64 && echo "Architecture: amd64" >> $1
}

export -f fix_arch_ctl
