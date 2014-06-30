# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../../scripts/common.sh

XBMC_MAN_PKGS="osmc-mediacenter"
XBMC_MAN_PKGS_RBP="osmc-mediacenter-rbp"
TOOLS_PKGS="wget apt-key"
CHROOT_PKGS="base-files"

export CHROOT_PKGS
export XBMC_MAN_PKGS
export XBMC_MAN_PKGS_RBP
