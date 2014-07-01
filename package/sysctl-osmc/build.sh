# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

echo -e "Building sysctl tweaks"
dpkg -b files/ osmc-sysctl.deb
