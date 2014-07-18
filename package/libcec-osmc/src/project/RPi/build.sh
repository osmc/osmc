#!/bin/bash

SCRIPT_PATH=`dirname $0`
cd $SCRIPT_PATH
SCRIPT_PATH=`pwd`
cd -

if [ -z "$1" ]; then
  DEST_DIR="/opt/libcec-rpi"
else
  DEST_DIR="$1"
fi

source $SCRIPT_PATH/config

$SCRIPT_PATH/get-toolchain.sh && \
$SCRIPT_PATH/build-deps.sh && \
$SCRIPT_PATH/bootstrap.sh $SCRIPT_PATH/toolchain .

if [ $? -eq 0 ]; then
  _set_toolchain_path "$SCRIPT_PATH/toolchain"
  # configure with --enable-rpi-cec-api so we bug out if we can't find Pi support or can't build it
  ./configure --host=$TARGET_HOST \
              --build=`cc -dumpmachine` \
              --prefix=$DEST_DIR \
              --enable-debug \
              --enable-rpi \
              --with-rpi-include-path="${SCRIPT_PATH}/firmware/hardfp/opt/vc/include" \
              --with-rpi-lib-path="${SCRIPT_PATH}/firmware/hardfp/opt/vc/lib" && \
  make clean && \
  make V=0

  exit $?
else
  exit 1
fi

