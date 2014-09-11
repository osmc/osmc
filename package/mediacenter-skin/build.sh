# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash
. ../common.sh

echo "Building OSMC skin tarball"
if [ ! -f /usr/bin/zip ]; then echo "Installing zip tools" && apt-get update && apt-get -y install zip; fi
pushd src
echo "Downloading dependencies"
pull_source "https://github.com/unfledged/script.skinshortcuts" "$(pwd)/script.skinshortcuts"
pull_source "https://github.com/BigNoid/service.library.data.provider" "$(pwd)/service.library.data.provider"
zip -r skin.osmc.zip *
popd
mv src/skin.osmc.zip .
echo "Build complete"
