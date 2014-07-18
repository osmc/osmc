# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

echo -e "Building base files for Jessie"
dpkg -b files/ base-files.deb
