# (c) 2014-2015 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../common.sh

makedirnb()
{
  # Git doesn't allow empty folders. Do at runtime.
  if [ ! -d files/${1} ]; then mkdir files/${1}; fi
}

echo -e "Building base files for Jessie"
make clean
makedirnb "proc"
makedirnb "bin"
makedirnb "tmp"
makedirnb "usr"
makedirnb "var"
makedirnb "run"
makedirnb "root"
makedirnb "sys"
makedirnb "lib"
makedirnb "opt"
makedirnb "home"
makedirnb "lib64"
makedirnb "mnt"
makedirnb "selinux"
makedirnb "boot"
makedirnb "dev"
dpkg_build files/ base-files-osmc.deb
