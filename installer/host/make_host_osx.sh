# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash
echo Building host installer for OS X
TARGET="qt_host_installer"
pushd ${TARGET}
if [ -f Makefile ]; then echo "Cleaning" && make clean; fi
echo Building installer
qmake
make
if [ $? != 0 ]; then echo "Build failed" && exit 1; fi
cp logo.icns ${TARGET}.app/Contents/Resources/
cp Info.plist ${TARGET}.app/Contents/
VERSION=$(cat ${TARGET}/${TARGET}.pro | grep VERSION | tail -n 1 | awk {'print $3'})
sed -e s/VERVAL/${VERSION} -i ${TARGET}.app/Contents/Info.plist
macdeployqt ${TARGET}.app -dmg -no-plugins
echo Packaging installer
popd
mv ${TARGET}/${TARGET}.dmg osmc-installer.dmg
echo Build complete
