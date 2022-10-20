#Host installer
This installer is run on the system that images the system on the target device. It runs on Windows, OS X and Linux.

Builds are performed statically:

* On Windows, Qt static builds are performed to remove any dependency issues. There are no external libraries.
* On Linux, Qt libraries are embedded into the executable. However, there are some external libraries: verify with ldd. The best to deploy is using the OpenSUSE implementation of Open Build Service (http://build.opensuse.org). This allows us to target multiple distributions without issue.
* On OSX: Qt libraries are embedded into the executable. However, again, some libraries are bundled externally.

## Linux static Qt build (for dev)

On Ubuntu or Debian, we can get a minimal build of Qt 4.8.x with the following commands:

`apt-get update`

`apt-get build-dep -y qt4-qmake`

`apt-get -y install build-essential git perl python libx11-dev "^libxb.*" libxrender-dev `

`git clone git://gitorious.org/qt/qt.git qt4 `

`cd qt4 `

```
./configure -release  -nomake examples -nomake demos -no-exceptions -no-stl -no-qt3support -no-scripttools -no-openssl -no-opengl -no-webkit -no-phonon -no-sql-sqlite -static -opensource -confirm-license --prefix=/usr
```

`make -j8 && make install`

## OS X Qt Static build 

Only tested on OS X Catalina:

Install Homebrew:

`/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"`

Install required brew packages:

`brew install pcre2 harfbuzz freetype coreutils binutils gnu-sed`

Download latest qt5 sourcecode from mirror,e.g:

`curl -O https://mirrors.ukfast.co.uk/sites/qt.io/archive/qt/5.15/5.15.6/single/qt-everywhere-opensource-src-5.15.6.tar.xz'

Then issue:
```
tar xf qt-everywhere-opensource-src-5.15.6.tar.xz
cd qt-everywhere-src-5.15.6
mkdir build && cd build
../configure -opensource -confirm-license -release --prefix=/usr/local -optimized-qmake -no-qml-debug -no-sql-sqlite -no-sql-db2 -no-sql-ibase -no-sql-mysql -no-sql-oci -no-sql-odbc -no-sql-psql -no-sql-sqlite -no-sql-sqlite2 -no-sql-tds -nomake examples -reduce-exports -static
`make -j8 && sudo make install`
```

## Windows Qt static build ##

Recommended: 32-bit Windows, as we don't get any benefit from x64 builds. We will build 32-bit Qt static libraries using MinGW32. You will also want to install WinRAR -- as we use this for creating the self-extracting tool which contains all the translations etc. 

* Download and install MinGW32, install it to C:\MinGW.
* Install the defaults: MSYS and GCC, G++ compilers
* Download the Qt-everywhere 4.8 source.
* Extract the source to C:\MinGW32\qt

Edit mkspecs\win32-g++\make.conf, ensure the CFLAGS_RELEASE line is:

````QMAKE_CFLAGS_RELEASE	= -Os -momit-leaf-frame-pointer````

Ensure we don't get issues with missing libgcc or libstdc++ DLLs by adding this line:

````QMAKE_LFLAGS		= -static````

Change

```QMAKE_CXXFLAGS		= $$QMAKE_CFLAGS```

to:

```QMAKE_CXXFLAGS		= $$QMAKE_CFLAGS -std=gnu++98```

Now, open a command prompt (cmd.exe). cd in to the C:\MinGW\qt\qt-everywhere-opensource-src-4.8.6 directory. Add MinGW binaries to your PATH:

````
PATH=%PATH%;C:\MinGW\bin
````

Now, let's configure:

````configure.exe -release  -nomake examples -nomake demos -no-exceptions -no-stl -no-rtti -no-qt3support -no-scripttools -no-openssl -no-opengl -no-webkit -no-phonon -no-style-motif -no-style-cde -no-style-cleanlooks -no-style-plastique -no-sql-sqlite -static -platform win32-g++ -qt-libpng -opensource -confirm-license````

Now, let's build:

````mingw32-make -j8````

Now, you have built Qt. Check make_host_win.sh to see where we expect everything to be in terms of PATHs, you may have to fix these.

You also need the manfiest embedding tool, or we can't embed a requireAdministrator manifest. Download the [Windows 7 SDK] (https://www.microsoft.com/en-gb/download/details.aspx?id=8279). Target: Platform does not really matter. You need .NET 2.0.

* Uncheck everything except for Tools
* This takes approx 200M disk. 

![Install options](https://raw.githubusercontent.com/osmc/osmc/master/installer/host/win_help1.png)

If you now have C:\Program Files\Microsoft SDKs\Windows\v7.1\Bin, you don't need to do anything. Otherwise, edit the PATH in make_host_win.sh again.

### Building

Now we can build any project by running 'make' followed by the platform name:

* win - produces a Windows MinGW static build
* osx - produces a OSX static DMG
* obs - produces a source tarball and necessary files for Debian and RPM generation

For Linux, we do packaging via OBS. However you can build a standard version with qmake && make. 
