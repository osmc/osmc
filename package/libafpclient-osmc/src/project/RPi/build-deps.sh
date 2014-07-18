#!/bin/bash

SCRIPT_PATH=`dirname $0`
cd $SCRIPT_PATH
SCRIPT_PATH=`pwd`
cd -

source $SCRIPT_PATH/config

mkdir -p $SCRIPT_PATH/deps/build
cd $SCRIPT_PATH/deps/build

if [ ! -d lockdev ]; then
  wget ${TARBALL_LOCATION}${LOCKDEV_TARBALL}
  tar -Jxf $LOCKDEV_TARBALL

  mv `echo $LOCKDEV_TARBALL | sed 's/.tar.xz//'` lockdev
  rm $LOCKDEV_TARBALL

  cd $SCRIPT_PATH/deps/build/lockdev && \
  $SCRIPT_PATH/bootstrap.sh $SCRIPT_PATH/toolchain .

  if [ $? -eq 0 ]; then
    _set_toolchain_path "$SCRIPT_PATH/toolchain"
    ./configure --host=$TARGET_HOST --build=`cc -dumpmachine` --prefix=$SCRIPT_PATH/deps && \
    make && \
    make install

    exit $?
  else
    exit 1
  fi
fi

exit 0

