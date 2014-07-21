arm-mem
=======

ARM-accelerated versions of selected functions from string.h

To build the library, use
$ make
or, if cross-compiling, use
$ CC=arm-linux-gnueabihf-gcc make

Also included is a simple test harness, inspired by the benchmarker
from the pixman library. This can be built via the "test" make target.
