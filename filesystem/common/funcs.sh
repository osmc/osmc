# (c) 2014 Sam Nazarko
# email@samnazarko.co.uk

#!/bin/bash

. ../../scripts/common.sh

XBMC_MAN_PKGS="osmc-mediacenter"
XBMC_MAN_PKGS_RBP="osmc-mediacenter-rbp"
SYSTEM_PKGS="wget apt-key base-files"
CHROOT_PKGS="{SYSTEM_PKGS}"

export CHROOT_PKGS
export XBMC_MAN_PKGS
export XBMC_MAN_PKGS_RBP
