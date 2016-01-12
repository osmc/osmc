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

Only tested on OS X El Capitan:

`git clone git://code.qt.io/qt/qt5.git -b 5.5.1`

`cd qt5 `

`./init-repository`

For configure, make sure you use a working `prefix`. El Capitan has a new security feature in place (http://enwp.org/System_Integrity_Protection). Usually, you want /usr/local anyway, but you might want to choose a different directory, to not mess up other Qt installs in /usr/local (like brew).
```
./configure -opensource -confirm-license -release --prefix=/usr/local -optimized-qmake -no-largefile -no-qml-debug -no-sql-sqlite -no-sql-db2 -no-sql-ibase -no-sql-mysql -no-sql-oci -no-sql-odbc -no-sql-psql -no-sql-sqlite -no-sql-sqlite2 -no-sql-tds -nomake examples -reduce-exports -static
```

Use a -j switch suitable for your processor. General rule of thumb: -jN where N is number of cores/processors +1.
Also, you may have to `sudo make install`, depending on the prefix used in the configure step 
`make -j8 && make install`

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
