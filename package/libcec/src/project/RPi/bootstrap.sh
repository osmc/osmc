#!/bin/bash

_usage()
{
  echo "Usage: $0 /path/to/toolchain /source/path"
  exit 1
}

if [[ -z "$2" || ! -d "$1" || ! -d "$2" ]]; then
  echo "1 = '$1'"
  echo "2 = '$2'"
  _usage
fi

SCRIPT_PATH=`dirname $0`
cd $SCRIPT_PATH
SCRIPT_PATH=`pwd`
cd -

source $SCRIPT_PATH/config
mkdir -p $SCRIPT_PATH/deps

cd "$2"

if [ -f "configure.ac" ]; then
  _set_toolchain_path "$1"
  autoreconf -vif
  exit $?
fi

exit 0

