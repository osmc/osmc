rem build script for win32
rem set the 
rem

rem EDIT THESE
set ONCRPC_BASE_DIR=C:\MinGW\msys\1.0\home\Administrator\src\oncrpc-win32\
set LIBNFS_BASE_DIR=..
rem END EDIT

set RPCINCLUDE="%ONCRPC_BASE_DIR%\win32\include"
set RPCDLL="%ONCRPC_BASE_DIR%\win32\bin\oncrpc.dll"
set RPCLIB="%ONCRPC_BASE_DIR%\win32\bin\oncrpc.lib"
set RPCGEN="%ONCRPC_BASE_DIR%\win32\bin\rpcgen.exe"


cd %LIBNFS_BASE_DIR%

rem generate NFS from .x
rem
copy nfs\nfs.x nfs\libnfs-raw-nfs.x
%RPCGEN% -h nfs\libnfs-raw-nfs.x > nfs\libnfs-raw-nfs.h
%RPCGEN% -c nfs\libnfs-raw-nfs.x > nfs\libnfs-raw-nfs.c
cl /I. /Iinclude /Iwin32 /I%RPCINCLUDE% -Zi -Od -c -DWIN32 -D_WIN32_WINNT=0x0501 -MDd nfs\libnfs-raw-nfs.c -Fonfs\libnfs-raw-nfs.obj
cl /I. /Iinclude /Iwin32 /I%RPCINCLUDE% -Zi -Od -c -DWIN32 -D_WIN32_WINNT=0x0501 -MDd nfs\nfs.c -Fonfs\nfs.obj
cl /I. /Iinclude /Iwin32 /I%RPCINCLUDE% -Zi -Od -c -DWIN32 -D_WIN32_WINNT=0x0501 -MDd nfs\nfsacl.c -Fonfs\nfsacl.obj



rem
rem generate RQUOTA from .x
rem
copy rquota\rquota.x rquota\libnfs-raw-rquota.x
%RPCGEN% -h rquota\libnfs-raw-rquota.x > rquota\libnfs-raw-rquota.h
%RPCGEN% -c rquota\libnfs-raw-rquota.x > rquota\libnfs-raw-rquota.c
cl /I. /Iinclude /Iwin32 /I%RPCINCLUDE% -Zi -Od -c -DWIN32 -D_WIN32_WINNT=0x0501 -MDd rquota\libnfs-raw-rquota.c -Forquota\libnfs-raw-rquota.obj
cl /I. /Iinclude /Iwin32 /I%RPCINCLUDE% -Zi -Od -c -DWIN32 -D_WIN32_WINNT=0x0501 -MDd rquota\rquota.c -Forquota\rquota.obj



rem
rem generate PORTMAP from .x
rem
copy portmap\portmap.x portmap\libnfs-raw-portmap.x
%RPCGEN% -h portmap\libnfs-raw-portmap.x > portmap\libnfs-raw-portmap.h
%RPCGEN% -c portmap\libnfs-raw-portmap.x > portmap\libnfs-raw-portmap.c
cl /I. /Iinclude /Iwin32 /I%RPCINCLUDE% -Zi -Od -c -DWIN32 -D_WIN32_WINNT=0x0501 -MDd portmap\libnfs-raw-portmap.c -Foportmap\libnfs-raw-portmap.obj
cl /I. /Iinclude /Iwin32 /I%RPCINCLUDE% -Zi -Od -c -DWIN32 -D_WIN32_WINNT=0x0501 -MDd portmap\portmap.c -Foportmap\portmap.obj


rem
rem generate MOUNT from .x
rem
copy mount\mount.x mount\libnfs-raw-mount.x
%RPCGEN% -h mount\libnfs-raw-mount.x > mount\libnfs-raw-mount.h
%RPCGEN% -c mount\libnfs-raw-mount.x > mount\libnfs-raw-mount.c
cl /I. /Iinclude /Iwin32 /I%RPCINCLUDE% -Zi -Od -c -DWIN32 -D_WIN32_WINNT=0x0501 -MDd mount\libnfs-raw-mount.c -Fomount\libnfs-raw-mount.obj
cl /I. /Iinclude /Iwin32 /I%RPCINCLUDE% -Zi -Od -c -DWIN32 -D_WIN32_WINNT=0x0501 -MDd mount\mount.c -Fomount\mount.obj



rem
rem generate core part of library
rem
cl /I. /Iinclude /Iwin32 /I%RPCINCLUDE% -Zi -Od -c -DWIN32 -D_WIN32_WINNT=0x0501 -MDd lib\init.c -Folib\init.obj
cl /I. /Iinclude /Iwin32 /I%RPCINCLUDE% -Zi -Od -c -DWIN32 -D_WIN32_WINNT=0x0501 -MDd -D_U_="" lib\pdu.c -Folib\pdu.obj
cl /I. /Iinclude /Iwin32 /I%RPCINCLUDE% -Zi -Od -c -DWIN32 -D_WIN32_WINNT=0x0501 -MDd -D_U_="" lib\socket.c -Folib\socket.obj
cl /I. /Iinclude /Iwin32 /I%RPCINCLUDE% /Imount /Infs -Zi -Od -c -DWIN32 -D_WIN32_WINNT=0x0501 -MDd -D_U_="" lib\libnfs.c -Folib\libnfs.obj
cl /I. /Iinclude /Iwin32 /I%RPCINCLUDE% /Imount /Infs -Zi -Od -c -DWIN32 -D_WIN32_WINNT=0x0501 -MDd -D_U_="" lib\libnfs-sync.c -Folib\libnfs-sync.obj

rem
rem generate win32 compat layer
rem
cl /I. -Zi -Od -c -DWIN32 -D_WIN32_WINNT=0x0501 -MDd -D_U_="" win32\win32_compat.c -Fowin32\win32_compat.obj



rem
rem create a linklibrary/dll
rem
lib /out:lib\libnfs.lib /def:lib\libnfs-win32.def nfs\nfs.obj nfs\nfsacl.obj nfs\libnfs-raw-nfs.obj rquota\rquota.obj rquota\libnfs-raw-rquota.obj mount\mount.obj mount\libnfs-raw-mount.obj portmap\portmap.obj portmap\libnfs-raw-portmap.obj lib\init.obj lib\pdu.obj lib\socket.obj lib\libnfs.obj lib\libnfs-sync.obj win32\win32_compat.obj

link /DLL /out:lib\libnfs.dll /DEBUG /DEBUGTYPE:cv lib\libnfs.exp nfs\nfs.obj nfs\nfsacl.obj nfs\libnfs-raw-nfs.obj rquota\rquota.obj rquota\libnfs-raw-rquota.obj mount\mount.obj mount\libnfs-raw-mount.obj portmap\portmap.obj portmap\libnfs-raw-portmap.obj lib\init.obj lib\pdu.obj lib\socket.obj lib\libnfs.obj lib\libnfs-sync.obj win32\win32_compat.obj %RPCLIB% ws2_32.lib



rem
rem build a test application
rem
cl /I. /Iinclude /Iwin32 /I%RPCINCLUDE% /Imount /Infs -Zi -Od -DWIN32 -D_WIN32_WINNT=0x0501 -MDd -D_U_="" examples\nfsclient-sync.c lib\libnfs.lib %RPCLIB% WS2_32.lib kernel32.lib mswsock.lib advapi32.lib wsock32.lib advapi32.lib








