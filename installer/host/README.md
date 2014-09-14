#Host installer
This installer is run on the system that images the system on the target device. It runs on Windows, OS X and Linux.

Builds are performed statically:

* On Windows, Qt static builds are performed to remove any dependency issues. Qt libraries and MinGW32 C++ libraries are embedded in an SFX.
* On Linux, Qt libraries are embedded into the executable. However, there are some external libraries: verify with ldd.
* On OSX: Qt libraries are embedded into the executable. However, again, some libraries are bundled externally.

## Linux static Qt build

On Ubuntu or Debian, we can get a minimal build of Qt 4.8.x with the following commands:

```apt-get update```

```apt-get build-dep -y qt4-qmake```

```apt-get -y install build-essential git perl python libx11-dev "^libxb.*" libxrender-dev ```

```git clone git://gitorious.org/qt/qt.git qt4 ```

```cd qt4 ```

```./configure -release  -nomake examples -nomake demos -no-exceptions -no-stl -no-qt3support -no-scripttools -no-openssl -no-opengl -no-webkit -no-phonon -no-sql-sqlite -static -opensource -confirm-license --prefix=/usr```

```make -j8 && make install```

## OS X Qt Static build 

Only tested on OS X Mavericks:

```git clone git://gitorious.org/qt/qt.git qt4 ```

```cd qt4 ```

```./configure -release  -nomake examples -nomake demos -no-exceptions -no-stl -no-qt3support -no-scripttools -no-openssl -no-opengl -no-webkit -no-phonon -no-sql-sqlite -static -opensource -confirm-license --prefix=/usr```

```make -j8 && make install```

## Windows Qt static build ##

We will build 32-bit Qt static libraries using MinGW32. You will also want to install WinRAR -- as we use this for creating the self-extracting tool which contains all the translations etc. 

* Download and install MinGW32, install it to C:\MinGW32.
* Install defaults: MSYS and GCC, G++ compilers
* Download the Qt-everywhere 4.8. source
* Extract the source to C:\MinGW32\Qt


### Building

Now we can build any project by running 'make' followed by the platform name:

* win
* linux
* osx


