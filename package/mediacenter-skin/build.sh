# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

echo "Building OSMC skin tarball"
if [ ! -f /usr/bin/zip ]; then echo "Installing zip tools" && apt-get update && apt-get -y install zip; fi
pushd src
zip -r skin.osmc.zip *
popd
mv src/skin.osmc.zip .
echo "Build complete"
