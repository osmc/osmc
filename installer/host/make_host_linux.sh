# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash
echo Building host installer for Linux
TARGET="qt_host_installer"
pushd ${TARGET}
if [ -f Makefile ]; then echo "Cleaning" && make clean; fi
echo Building installer
qmake
make
strip ${TARGET}
echo Packaging installer
popd
INSTALL="install"
if [ -d ${INSTALL} ]; then echo "Cleaning old install directory " && rm -rf ${INSTALL}; fi
mkdir -p ${INSTALL}
mkdir -p ${INSTALL}/usr/bin/osmc
mkdir -p ${INSTALL}/usr/share/applications
cp ${TARGET}/${TARGET} ${INSTALL}/usr/bin/osmc/
cp ${TARGET}/*.qm ${INSTALL}/usr/bin/osmc/ > /dev/null 2>&1
cp ${TARGET}/osmcinstaller.desktop ${INSTALL}/usr/share/applications/
cp ${TARGET}/icon.png ${INSTALL}/usr/bin/osmc/icon.png
cp -ar DEBIAN/ ${INSTALL}/
VERSION=$(cat ${TARGET}/${TARGET}.pro | grep VERSION | tail -n 1 | awk {'print $3'})
echo "Version: ${VERSION}" >> ${INSTALL}/DEBIAN/control
test $(arch)x == x86_64x && echo "Architecture: amd64" >> ${INSTALL}/DEBIAN/control
test $(arch)x == i686x && echo "Architecture: i686" >> ${INSTALL}/DEBIAN/control
dpkg -b ${INSTALL} osmc-installer-$(arch).deb
rm -rf ${INSTALL}
echo Build complete
