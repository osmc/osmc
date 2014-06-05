# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

function check_platform()
{
platform=$(lsb_release -a | grep Debian)
echo $platform | grep -q wheezy
if [ $? != 0 ]; then echo -e "We are not running Debian Wheezy" && return 1; else return 0; fi
}

function verify_action()
{
	if [ $? != 0 ]; then echo -e "Exiting build" && exit 1; fi
}

function enable_nw_chroot()
{
	echo -e "Enabling networking"
	FILES="/etc/resolv.conf
	/etc/network/interfaces"
	for file in $FILES
	do
	if [ ! -f $file ]
	then
	echo "Couldn't find network file ${1}. \nchroot network support will likely fail"
	else
	cp $file $1
	fi
	done
}

export -f check_platform
export -f verify_action
export -f enable_nw_chroot
