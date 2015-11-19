# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

# Run make win from MSYS

#!/bin/bash
echo Building host installer for Windows via MSYS
QT_VER="4.8.6"
QT_PATH="/c/MinGW/qt/qt-everywhere-opensource-src-${QT_VER}/bin"
SDK_PATH="/c/Program Files/Microsoft SDKs/Windows/v7.1/Bin"
RAR_PATH="/c/Program Files/WinRAR/"
MINGW_PATH="/c/MinGW/bin"
echo -e "Updating PATH"
PATH="${PATH}:${QT_PATH}:${SDK_PATH}:${RAR_PATH}:${MINGW_PATH}"
TARGET="qt_host_installer"
ZLIB_VER="1.2.8"
pushd ${TARGET}
VERSION=$(cat ${TARGET}.pro | grep VERSION | tail -n 1 | awk {'print $3'})
if [ -f Makefile ]; then
	echo "Cleaning Qt project"
	mingw32-make clean
	if [ $? != 0 ]; then echo "Clean failed"; exit 1; fi
fi
pushd w32-lib/zlib-${ZLIB_VER}
make -f win32/Makefile.gcc clean
popd
echo Building zlib version ${ZLIB_VER}
pushd w32-lib/zlib-${ZLIB_VER}
make -f win32/Makefile.gcc
if [ $? != 0 ]; then echo "Building zlib failed" && exit 1; fi
popd
echo Building installer
qmake
mingw32-make
if [ $? != 0 ]; then echo "Building project failed" && exit 1; fi
#strip release/${TARGET}.exe
echo Packaging installer
popd
INSTALL="install"
if [ -d ${INSTALL} ]; then echo "Cleaning old install directory " && rm -rf ${INSTALL}; fi
mkdir -p ${INSTALL}
cp ${TARGET}/release/${TARGET}.exe ${INSTALL}/
cp ${TARGET}/*.qm ${INSTALL}/ > /dev/null 2>&1
cp ${TARGET}/winrar.sfx ${INSTALL}
echo Building manifest
mt.exe -manifest qt_host_installer/qt_host_installer.exe.manifest -outputresource:install/qt_host_installer.exe
pushd ${INSTALL}
Rar.exe a -r -sfx -z"winrar.sfx" osmc-installer qt_host_installer.exe *.qm >/dev/null 2>&1
popd
mv ${INSTALL}/osmc-installer.exe .
rm -rf ${INSTALL}
umount /qtbin >/dev/null 2>&1
umount /mgwbin >/dev/null 2>&1
# Update on server
echo ${VERSION} > latest_windows
echo Build complete
