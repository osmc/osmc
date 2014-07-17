LibNFS requires a oncrpc library and rpcgen compiler that can handle
64 bit types. Link below for one such package, but any 64bit capable oncrpc
package should work with some effort.


1. checkout git://github.com/Memphiz/oncrpc-win32.git project (branch hyper!)
2. build oncrpc-win32 project (see README.libnfs in the projects dir)
3. checkout libnfs ("parallel" to oncrpc-win32)
4. load vs2010 project from win32/libnfs/libnfs.sln
5. build libnfs and nfsclient-sync
