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
cp resources/osx/logo.icns ${TARGET}.app/Contents/Resources/.	
cp resources/osx/Info.plist ${TARGET}.app/Contents/.
macdeployqt ${TARGET}.app -dmg -no-plugins
echo Packaging installer
popd
mv ${TARGET}/${TARGET}.dmg osmc-installer.dmg
echo Build complete
