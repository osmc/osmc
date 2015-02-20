# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../../scripts/common.sh

echo -e "Building JSON App Parser"

echo -e "Installing dependencies"
update_sources
verify_action
install_package build-essential
verify_action
gcc application-parser.c -o /usr/bin/application-parser
echo -e "Build complete"
