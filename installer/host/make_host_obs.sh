# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash
echo Building installer files for OBS

TARGET="qt_host_installer"
OUTPUT="obs"
pushd ${TARGET}
if [ -f Makefile ]; then echo "Cleaning" && make clean; fi
rm Makefile >/dev/null 2>&1
VERSION=$(cat ${TARGET}.pro | grep VERSION | tail -n 1 | awk {'print $3'})
popd
mkdir ${OUTPUT}
mkdir ${OUTPUT}/src
cp -ar ${TARGET}/* ${OUTPUT}/src/
pushd ${OUTPUT}
echo Creating tarball
tar -czf src.tar.gz src/
rm -rf src/
popd
cp ${TARGET}.spec ${OUTPUT}
echo Updating RPM versioning
sed -e s/PLACEHOLDER/${VERSION}/ -i ${OUTPUT}/${TARGET}.spec
echo Updating Debian versioning
cp debian.changelog debian.control debian.dsc debian.rules debian_change_desktop.patch ${OUTPUT}
sed -e s/PLACEHOLDER/${VERSION}/ -i ${OUTPUT}/debian.changelog
sed -e s/PLACEHOLDER/${VERSION}/ -i ${OUTPUT}/debian.dsc
echo Files are now ready for OBS
