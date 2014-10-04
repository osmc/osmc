# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash
echo Building host installer for RPM
TARGET="qt_host_installer"
tar -czf ${TARGET}.tar.gz ${TARGET}
rm ~/rpmbuild/SOURCES/${TARGET}.tar.gz
rm ~/rpmbuild/SOURCES/*.desktop
mv ${TARGET}.tar.gz ~/rpmbuild/SOURCES
rpmbuild -bb ${TARGET}/rpm.spec
if [ $? != 0 ]; then echo "Creating RPM failed" && exit 1; fi
echo Build complete
