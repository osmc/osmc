#!/bin/bash

SCRIPT_PATH=`dirname $0`
cd $SCRIPT_PATH
SCRIPT_PATH=`pwd`
cd -

source $SCRIPT_PATH/config

if [ ! -d $SCRIPT_PATH/toolchain ]; then
  git clone $TOOLCHAIN_GIT $SCRIPT_PATH/toolchain
else
  cd $SCRIPT_PATH/toolchain
#  git pull
fi

if [ ! -d $SCRIPT_PATH/firmware ]; then
  git clone $FIRMWARE_GIT $SCRIPT_PATH/firmware
else
  cd $SCRIPT_PATH/firmware
#  git pull
fi

if [[ -d $SCRIPT_PATH/toolchain && -d $SCRIPT_PATH/firmware ]]; then
  exit 0
else
  exit 1
fi

