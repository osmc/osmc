# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

function fix_arch_ctl()
{
	sed '/Architecture/d' $1
	test $(arch)x == i686x && echo "Architecture: i386" >> $1
	test $(arch)x == armhfx && echo "Architecture: armhf" >> $1
	test $(arch)x == amd64 && echo "Architecture: amd64" >> $1
}

export -f fix_arch_ctl
