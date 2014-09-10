# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

echo "Building OSMC skin tarball"
pushd src
zip -r skin.osmc.zip *
popd
mv src/skin.osmc.zip .
echo "Build complete"
