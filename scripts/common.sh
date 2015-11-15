# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

function check_platform()
{
    platform=$(lsb_release -c -s)
    case $platform in
        "wheezy" | "trusty" | "utopic" | "jessie" | "wily" )
            return 0
            ;;
        * )
            return 1
    esac
}

function verify_action()
{
	if [ $? != 0 ]; then echo -e "Exiting build" && exit 1; fi
}

function enable_nw_chroot()
{
	echo -e "Enabling networking"
	cp /etc/resolv.conf $1/etc/
	if [ $? != 0 ]; then echo -e "Can't copy networking file" && return 1; fi
	cp /etc/network/interfaces $1/etc/network
	if [ $? != 0 ]; then echo -e "Can't copy networking file" && return 1; fi
}

function add_apt_key()
{
	echo -e "Adding apt key"
	wget ${2} -O ${1}/tmp/key
	chroot ${1} apt-key add /tmp/key
	rm ${1}/tmp/key > /dev/null 2>&1
}

function emulate_arm()
{
	echo Copying ARM QEMU binary
	cp /usr/bin/qemu-arm-static ${1}/usr/bin/qemu-arm-static
}

function remove_emulate_arm()
{
	echo Removing ARM QEMU binary
	rm ${1}/usr/bin/qemu-arm-static
}

function update_sources()
{
	echo -e "Updating sources"
	apt-get update > /dev/null 2>&1
	if [ $? != 0 ]; then echo -e "Failed to update sources" && return 1; else echo -e "Sources updated successfully" && return 0; fi
}

function install_package()
{
	echo -e "Installing package ${1}..."
	# Check if our package is installed
	# Although this may seem duplicated in handle_dep. handle_dep is used for packages only, where as installers/ and other parts will call this function directly. handle_dep purely exists to tell us when we need to build first or add an apt repo.
	if dpkg-query -W -f='${Status}' "${1}" 2>/dev/null | grep -q "ok installed" >/dev/null 2>&1
	then
		echo -e "Package already installed."
	else
		apt-get -y --no-install-recommends install $1
		if [ $? != 0 ]; then echo -e "Failed to install" && return 1; else echo -e "Package installed successfully" && return 0; fi
	fi
}

function fetch_filesystem()
{
	echo -e "Fetching base filesystem for building target\nPlease be patient"
	debootstrap $1
	if [ $? == 0 ]
	then
	echo -e "Filesystem base install successful"
	return 0
	else
	echo -e "Filesystem base install failed"
	return 1
	fi
}

function configure_filesystem()
{
	echo -e "Configuring filesystem\nPlease be patient"
	chroot $1 /debootstrap/debootstrap --second-stage
	if [ $? == 0 ]
	then
	echo -e "Filesystem configured successfully"
	return 0
	else
	echo -e "Filesystem configuration failed"
	fi
}

function cleanup_filesystem()
{
	echo -e "Cleaning up filesystem"
	rm -f ${1}/etc/resolv.conf
	rm -f ${1}/etc/network/interfaces
	rm -rf ${1}/usr/share/man/* 
	rm -rf ${1}/var/cache/apt/archives/*
	rm -rf ${1}/var/lib/apt/lists/*
	rm -f ${1}/var/log/*.log
	rm -f ${1}/var/log/apt/*.log
	rm -f ${1}/tmp/reboot-needed
	rm -f ${1}/var/cache/apt/pkgcache.bin
}

function enable_mirrordirector()
{
	echo -e "Enabling mirror direction"
	sed -e s/staging.//g -i ${1}/etc/apt/sources.list
}

function remove_existing_filesystem()
{
	if [ -d "$1" ]; then echo -e "Removing old filesystem" && rm -rf "$1"; fi
}

function install_patch()
{
	patches=$(find ${1} -name "${2}-*.patch" -printf '%P\n' | sort)
	for patch in $patches
	do
		cp ${1}/$patch .
		echo Applying patch $patch
		if ! grep -q "GIT binary patch" $patch
		then
		    patch -p1 --ignore-whitespace < $patch
		    verify_action
	        else
		    # this is a binary patch
		    install_package "git"
		    verify_action
		    prefix=$(realpath --relative-to="$(git rev-parse --show-toplevel)" .)
		    git apply --whitespace=nowarn --directory="$prefix" $patch
		    verify_action
	        fi
	        rm $patch
	done
}

function pull_source()
{
	ischroot
	chrootval=$?
	if [ $chrootval == 2 ] || [ $chrootval == 0 ]; then return; fi # Prevent recursive loop
	if ! command -v unzip >/dev/null 2>&1; then update_sources && verify_action && install_package "unzip" && verify_action; fi
	if ! command -v git >/dev/null 2>&1; then update_sources && verify_action && install_package "git" && verify_action; fi
	if ! command -v svn >/dev/null 2>&1; then update_sources && verify_action && install_package "subversion" && verify_action; fi
	if ! command -v wget >/dev/null 2>&1; then update_sources && verify_action && install_package "wget" && verify_action; fi
	if [ -d ${2} ]
	then 
		if [ "$2" != "." ]
		then
			echo "Cleaning old source" && rm -rf ${2}; fi
		fi
	if [[ $1 =~ \.zip$ ]]
	then
	echo -e "Detected ZIP source"
	if [ "$2" != "." ]; then mkdir -p ${2}; fi
	wget ${1} -O source.zip
	if [ $? != 0 ]; then echo "Downloading zip failed" && exit 1; fi
	unzip source.zip -d ${2}
	rm source.zip
	return
	fi

	if [[ $1 =~ \.tar$ || $1 =~ \.tgz$ || $1 =~ \.tar\.gz$ || $1 =~ \.tar\.bz2$ || $1 =~ \.tar\.xz$ ]]
	then
	echo -e "Detected tarball source"
	if [ "$2" != "." ]; then mkdir -p ${2}; fi
	wget ${1} -O source.tar
	if [ $? != 0 ]; then echo "Downloading tarball failed" && exit 1; fi
	tar -xvf source.tar -C ${2}
	rm source.tar
	return
	fi

	if [[ $1 =~ svn ]]
	then
	echo -e "Detected SVN source"
	svn co ${1} ${2}
	if [ $? != 0 ]; then echo "Source checkout failed" && exit 1; fi
	return
	fi

	if [[ $1 =~ git ]]
	then
	echo -e "Detected Git source"
	git clone ${1} ${2}
	if [ $? != 0 ]; then echo "Source checkout failed" && exit 1; fi
	return
	fi

	echo -e "No file type match found for URL" && exit 1
}

function pull_bin()
{
	ischroot
	chrootval=$?
	if [ $chrootval == 2 ] || [ $chrootval == 0 ]; then return; fi # Prevent recursive loop
	if ! command -v wget >/dev/null 2>&1; then update_sources && verify_action && install_package "wget" && verify_action; fi
	if [ -f $2 ]; then echo "Cleaning old source" && rm -f ${2}; fi
	wget ${1} -O ${2}
	if [ $? != 0 ]; then echo "Binary download failed" && exit 1; fi
	chmod +x ${2}
}

if [ -z $DOWNLOAD_URL ]
then
	DOWNLOAD_URL=$(env LANG=C wget -S --spider --timeout 60 http://download.osmc.tv 2>&1 > /dev/null | grep "^Location:" | cut -f 2 -d ' ')
	export DOWNLOAD_URL
fi

cores=$(if [ ! -f /proc/cpuinfo ]; then mount -t proc proc /proc; fi; cat /proc/cpuinfo | grep processor | wc -l && umount /proc/ >/dev/null 2>&1)
export BUILD="make -j${cores}"

export -f check_platform
export -f verify_action
export -f enable_nw_chroot
export -f add_apt_key
export -f emulate_arm
export -f remove_emulate_arm
export -f update_sources
export -f install_package
export -f fetch_filesystem
export -f cleanup_filesystem
export -f enable_mirrordirector
export -f remove_existing_filesystem
export -f install_patch
export -f pull_source
export -f pull_bin
