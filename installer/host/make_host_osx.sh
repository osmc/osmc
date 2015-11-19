# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash
echo Building host installer for OS X

TARGET="qt_host_installer"
RESOURCES="resources"
ICONDIR="${RESOURCES}/app.iconset"
ICONSET_NAME="logo.icns"

pushd ${TARGET}
if [ -f Makefile ]; then
	echo "Cleaning"
	make clean
	if [ $? != 0 ]; then echo "Clean failed"; exit 1; fi
fi
rm -rf ${TARGET}.app
echo Building installer
qmake
make
if [ $? != 0 ]; then echo "Build failed" && exit 1; fi

cp ${RESOURCES}/Info.plist ${TARGET}.app/Contents/

## handle application icons
echo Generating icons
if command -v iconutil >/dev/null 2>&1
then
    rm ${RESOURCES}/${ICONSET_NAME}
    iconutil -c icns --output ${RESOURCES}/${ICONSET_NAME} ${ICONDIR}
else
    echo "This system does not have iconutil. Using old icons"
fi
cp ${RESOURCES}/${ICONSET_NAME} ${TARGET}.app/Contents/Resources/
sed -e s/ICON_HERE/${ICONSET_NAME}/ -i old ${TARGET}.app/Contents/Info.plist
echo Placing Version

## try to set version in plist
VERSION=$(cat ${TARGET}.pro | grep VERSION | tail -n 1 | awk {'print $3'})
sed -e s/VERVAL/${VERSION}/ -i old ${TARGET}.app/Contents/Info.plist
# Update on server
echo Packaging installer
macdeployqt ${TARGET}.app -dmg
popd
echo ${VERSION} > latest_mac
mv ${TARGET}/${TARGET}.dmg osmc-installer.dmg
echo Build complete
