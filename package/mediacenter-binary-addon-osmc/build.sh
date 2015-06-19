# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

git_to_archive()
{
    file_contents=$(cat $1)
    if grep -q github.com $1
    then
	PKG_NAME=$(echo $file_contents | cut -f 1 -d " ")
        GIT_REPO=$(echo $file_contents | cut -f 2 -d " ")
        GIT_REV=$(echo $file_contents | cut -f 3 -d " ")
        GIT_URL=$(echo ${GIT_REPO}/archive/${GIT_REV}.zip)
        echo "${PKG_NAME} ${GIT_URL}" > $1
     fi
}

if [ -z "$2" ]
then
    echo -e "Please specify which addons you would like to build"
    exit 1
fi
pull_source "https://github.com/xbmc/xbmc/archive/master.tar.gz" "$(pwd)/src"
if [ $? != 0 ]; then echo -e "Error downloading" && exit 1; fi
# Build in native environment
build_in_env "${1}" $(pwd) "mediacenter-binary-addon-osmc" "ADDONS=${2}"
build_return=$?
if [ $build_return == 99 ]
then
	echo -e "Building binary addons for mediacenter"
	out=$(pwd)/files
	if [ -d files/usr ]; then rm -rf files/usr; fi
	update_sources
	handle_dep "cmake"
	handle_dep "git"
	handle_dep "make"
	pushd src/xbmc-*
	install_patch "../../patches" "all"
	for file in project/cmake/addons/addons/*/*.txt
	do
	    git_to_archive "$file"
	done
	git_to_archive "project/cmake/addons/depends/common/kodi-platform/kodi-platform.txt"
	popd
	if [ "$2" == "all" ]
	then
	    ADDONS="pvr.argustv pvr.dvblink pvr.dvbviewer pvr.filmon pvr.hts pvr.iptvsimple pvr.mediaportal pvr.mythtv pvr.nextpvr pvr.njoy pvr.pctv pvr.stalker pvr.vbox pvr.vdr pvr.vuplus pvr.wmc audioencoder.wav audioencoder.vorbis audioencoder.lame audioencoder.flac audioencoder.vgmstream"
	else
	    ADDONS="$2"
	fi
	for ADDON in $ADDONS
	do
		sed '/Package/d' -i files/DEBIAN/control
		sed '/Version/d' -i files/DEBIAN/control
		sed '/Replaces/d' -i files/DEBIAN/control
		sed '/Breaks/d' -i files/DEBIAN/control
		echo "Package: ${1}-mediacenter-binary-addon-${ADDON}-osmc" >> files/DEBIAN/control
		pushd src/xbmc-*
		$BUILD -C tools/depends/target/binary-addons/ INSTALL_OSMC_DIR="${out}/usr" PREFIX="." ADDONS="$ADDON"
		if [ $? != 0 ]; then echo "Error occured during build" && exit 1; fi
		strip_files "${out}"
		popd
		VERSION_OSMC=1
		VERSION_ADDON=$(grep version ${out}/usr/share/kodi/addons/${ADDON}/addon.xml | head -n 2 | tail -n 1 | cut -f 2 -d \")
		echo "Version: ${VERSION_ADDON}-${VERSION_OSMC}" >> files/DEBIAN/control
		echo ${ADDON} | grep -q pvr
		BREAKS_HELIX=$?
		if [ "$BREAKS_HELIX" -eq 0 ]
		then
			if [ "$1" == "armv6l" ]
			then
				echo "Replaces: rbp1-mediacenter-osmc (<< 14.9.0)" >> files/DEBIAN/control
				echo "Breaks: rbp1-mediacenter-osmc (<< 14.9.0), vero-mediacenter-osmc (<< 14.9.0)" >> files/DEBIAN/control

			fi
			if [ "$1" == "armv7" ]
			then
				echo "Replaces: rbp2-mediacenter-osmc (<< 14.9.0), vero-mediacenter-osmc (<< 14.9.0)" >> files/DEBIAN/control
				echo "Breaks: rbp2-mediacenter-osmc (<< 14.9.0), vero-mediacenter-osmc (<< 14.9.0)" >> files/DEBIAN/control
			fi
		fi
		fix_arch_ctl "files/DEBIAN/control"
		dpkg_build files ${1}-mediacenter-binary-addon-${ADDON}-osmc.deb
		rm -rf ${out}/usr # Clean for next addon
	done
	build_return=$?
fi
teardown_env "${1}"
exit $build_return
