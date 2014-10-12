# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash
echo Building installer files for OBS

TARGET="qt_host_installer"
OUTPUT="obs"
pushd ${TARGET}
if [ -f Makefile ]; then echo "Cleaning" && make clean; fi
rm Makefile 
echo Creating tarball
tar -czvf src.tar.gz *
VERSION=$(cat ${TARGET}.pro | grep VERSION | tail -n 1 | awk {'print $3'})
popd
mkdir ${OUTPUT}
cp ${TARGET}/src.tar.gz ${OUTPUT}
cp ${TARGET}.spec ${OUTPUT}
echo Updating RPM versioning
sed -e s/PLACEHOLDER/${VERSION}/ -i ${OUTPUT}/${TARGET}.spec
echo Updating Debian versioning
#TBC
echo Files are now ready for OBS
