/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#ifndef TARGET_POSIX
#include <io.h>
#include <direct.h>
#include <process.h>
#else
#if !defined(TARGET_DARWIN) && !defined(TARGET_FREEBSD)
#include <mntent.h>
#endif
#endif
#include <sys/stat.h>
#include <sys/types.h>
#if !defined(TARGET_FREEBSD)
#include <sys/timeb.h>
#endif
#include "system.h" // for HAS_DVD_DRIVE
#ifdef HAS_DVD_DRIVE
  #ifdef TARGET_POSIX
    #include <sys/ioctl.h>
    #if defined(TARGET_DARWIN)
      #include <IOKit/storage/IODVDMediaBSDClient.h>
    #elif !defined(TARGET_FREEBSD)
      #include <linux/cdrom.h>
    #endif
  #endif
#endif
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#ifdef TARGET_POSIX
#include "PlatformDefs.h" // for __stat64
#endif
#include "Util.h"
#include "filesystem/SpecialProtocol.h"
#include "URL.h"
#include "filesystem/File.h"
#include "settings/Settings.h"
#include "FileItem.h"
#include "filesystem/Directory.h"

#include "emu_msvcrt.h"
#include "emu_dummy.h"
#include "emu_kernel32.h"
#include "util/EmuFileWrapper.h"
#include "utils/log.h"
#include "threads/SingleLock.h"
#ifndef TARGET_POSIX
#include "utils/CharsetConverter.h"
#include "utils/URIUtils.h"
#endif
#if defined(TARGET_ANDROID)
#include "android/loader/AndroidDyload.h"
#elif !defined(TARGET_WINDOWS)
#include <dlfcn.h>
#endif
#include "utils/Environment.h"
#include "utils/StringUtils.h"

using namespace XFILE;

struct SDirData
{
  CFileItemList items;
  int curr_index;
  struct dirent *last_entry;
  SDirData()
  {
    curr_index = -1;
    last_entry = NULL;
  }
};

#define MAX_OPEN_DIRS 10
static SDirData vecDirsOpen[MAX_OPEN_DIRS];
bool bVecDirsInited = false;
#ifdef HAS_VIDEO_PLAYBACK
extern void update_cache_dialog(const char* tmp);
#endif

#define EMU_MAX_ENVIRONMENT_ITEMS 100
static char *dll__environ_imp[EMU_MAX_ENVIRONMENT_ITEMS + 1];
extern "C" char **dll__environ;
char **dll__environ = dll__environ_imp;

CCriticalSection dll_cs_environ;

extern "C" void __stdcall init_emu_environ()
{
  memset(dll__environ, 0, EMU_MAX_ENVIRONMENT_ITEMS + 1);

  // python
#if defined(TARGET_WINDOWS)
  // fill our array with the windows system vars
  LPTSTR lpszVariable; 
  LPTCH lpvEnv;
  lpvEnv = GetEnvironmentStrings();
  if (lpvEnv != NULL)
  {
    lpszVariable = (LPTSTR) lpvEnv;
    while (*lpszVariable)
    {
      dll_putenv(lpszVariable);
      lpszVariable += lstrlen(lpszVariable) + 1;
    }
    FreeEnvironmentStrings(lpvEnv);
  }
  dll_putenv("OS=win32");
#elif defined(TARGET_DARWIN)
  dll_putenv("OS=darwin");
#elif defined(TARGET_POSIX)
  dll_putenv("OS=linux");
#else
  dll_putenv("OS=unknown");
#endif

  // check if we are running as real xbmc.app or just binary
  if (!CUtil::GetFrameworksPath(true).empty())
  {
    // using external python, it's build looking for xxx/lib/python2.6
    // so point it to frameworks which is where python2.6 is located
    dll_putenv(std::string("PYTHONPATH=" +
      CSpecialProtocol::TranslatePath("special://frameworks")).c_str());
    dll_putenv(std::string("PYTHONHOME=" +
      CSpecialProtocol::TranslatePath("special://frameworks")).c_str());
    dll_putenv(std::string("PATH=.;" +
      CSpecialProtocol::TranslatePath("special://xbmc") + ";" +
      CSpecialProtocol::TranslatePath("special://frameworks")).c_str());
  }
  else
  {
    dll_putenv(std::string("PYTHONPATH=" +
      CSpecialProtocol::TranslatePath("special://xbmc/system/python/DLLs") + ";" +
      CSpecialProtocol::TranslatePath("special://xbmc/system/python/Lib")).c_str());
    dll_putenv(std::string("PYTHONHOME=" +
      CSpecialProtocol::TranslatePath("special://xbmc/system/python")).c_str());
    dll_putenv(std::string("PATH=.;" + CSpecialProtocol::TranslatePath("special://xbmc") + ";" +
      CSpecialProtocol::TranslatePath("special://xbmc/system/python")).c_str());
  }

#if defined(TARGET_ANDROID)
  std::string apkPath = getenv("XBMC_ANDROID_APK");
  apkPath += "/assets/python2.6";
  dll_putenv(std::string("PYTHONHOME=" + apkPath).c_str());
  dll_putenv("PYTHONOPTIMIZE=");
  dll_putenv("PYTHONNOUSERSITE=1");
  dll_putenv("PYTHONPATH=");
#else
  dll_putenv("PYTHONOPTIMIZE=1");
#endif

  //dll_putenv("PYTHONCASEOK=1");
  //dll_putenv("PYTHONDEBUG=1");
  //dll_putenv("PYTHONVERBOSE=2"); // "1" for normal verbose, "2" for more verbose ?
  //dll_putenv("PYTHONDUMPREFS=1");
  //dll_putenv("THREADDEBUG=1");
  //dll_putenv("PYTHONMALLOCSTATS=1");
  //dll_putenv("PYTHONY2K=1");
  dll_putenv("TEMP=special://temp/temp"); // for python tempdir

  // libdvdnav
  dll_putenv("DVDREAD_NOKEYS=1");
  //dll_putenv("DVDREAD_VERBOSE=1");
  //dll_putenv("DVDREAD_USE_DIRECT=1");

  // libdvdcss
  dll_putenv("DVDCSS_METHOD=key");
  dll_putenv("DVDCSS_VERBOSE=3");
  dll_putenv("DVDCSS_CACHE=special://masterprofile/cache");
}

extern "C" void __stdcall update_emu_environ()
{
  // Use a proxy, if the GUI was configured as such
  if (CSettings::GetInstance().GetBool(CSettings::SETTING_NETWORK_USEHTTPPROXY)
      && !CSettings::GetInstance().GetString(CSettings::SETTING_NETWORK_HTTPPROXYSERVER).empty()
      && CSettings::GetInstance().GetInt(CSettings::SETTING_NETWORK_HTTPPROXYPORT) > 0
      && CSettings::GetInstance().GetInt(CSettings::SETTING_NETWORK_HTTPPROXYTYPE) == 0)
  {
    std::string strProxy;
    if (!CSettings::GetInstance().GetString(CSettings::SETTING_NETWORK_HTTPPROXYUSERNAME).empty() &&
        !CSettings::GetInstance().GetString(CSettings::SETTING_NETWORK_HTTPPROXYPASSWORD).empty())
    {
      strProxy = StringUtils::Format("%s:%s@",
                                     CSettings::GetInstance().GetString(CSettings::SETTING_NETWORK_HTTPPROXYUSERNAME).c_str(),
                                     CSettings::GetInstance().GetString(CSettings::SETTING_NETWORK_HTTPPROXYPASSWORD).c_str());
    }

    strProxy += CSettings::GetInstance().GetString(CSettings::SETTING_NETWORK_HTTPPROXYSERVER);
    strProxy += StringUtils::Format(":%d", CSettings::GetInstance().GetInt(CSettings::SETTING_NETWORK_HTTPPROXYPORT));

    CEnvironment::setenv( "HTTP_PROXY", "http://" + strProxy, true );
    CEnvironment::setenv( "HTTPS_PROXY", "http://" + strProxy, true );
  }
  else
  {
    // is there a better way to delete an environment variable?
    // this works but leaves the variable
    dll_putenv( "HTTP_PROXY=" );
    dll_putenv( "HTTPS_PROXY=" );
  }
}

extern "C" void __stdcall cleanup_emu_environ()
{
  for (int i = 0; i < EMU_MAX_ENVIRONMENT_ITEMS; i++)
  {
    free(dll__environ[i]);
    dll__environ[i] = NULL;
  }
}

static int convert_fmode(const char* mode)
{
  int iMode = O_BINARY;
  if (strstr(mode, "r+"))
    iMode |= O_RDWR;
  else if (strchr(mode, 'r'))
    iMode |= _O_RDONLY;
  if (strstr(mode, "w+"))
    iMode |= O_RDWR | _O_TRUNC;
  else if (strchr(mode, 'w'))
    iMode |= _O_WRONLY  | O_CREAT;
  return iMode;
}

#ifdef TARGET_WINDOWS
static void to_finddata64i32(_wfinddata64i32_t *wdata, _finddata64i32_t *data)
{
  std::string strname;
  g_charsetConverter.wToUTF8(wdata->name, strname);
  size_t size = sizeof(data->name) / sizeof(char);
  strncpy(data->name, strname.c_str(), size);
  if (size)
    data->name[size - 1] = '\0';
  data->attrib = wdata->attrib;
  data->time_create = wdata->time_create;
  data->time_access = wdata->time_access;
  data->time_write = wdata->time_write;
  data->size = wdata->size;
}

static void to_wfinddata64i32(_finddata64i32_t *data, _wfinddata64i32_t *wdata)
{
  std::wstring strwname;
  g_charsetConverter.utf8ToW(data->name, strwname, false);
  size_t size = sizeof(wdata->name) / sizeof(wchar_t);
  wcsncpy(wdata->name, strwname.c_str(), size);
  if (size)
    wdata->name[size - 1] = '\0';
  wdata->attrib = data->attrib;
  wdata->time_create = data->time_create;
  wdata->time_access = data->time_access;
  wdata->time_write = data->time_write;
  wdata->size = data->size;
}
#endif

extern "C"
{
  void dll_sleep(unsigned long imSec)
  {
    Sleep(imSec);
  }

  // FIXME, XXX, !!!!!!
  void dllReleaseAll( )
  {
    // close all open dirs...
    if (bVecDirsInited)
    {
      for (int i=0;i < MAX_OPEN_DIRS; ++i)
      {
        vecDirsOpen[i].items.Clear();
      }
      bVecDirsInited = false;
    }
  }

  void* dllmalloc(size_t size)
  {
    void* pBlock = malloc(size);
    if (!pBlock)
    {
      CLog::Log(LOGSEVERE, "malloc %" PRIdS" bytes failed, crash imminent", size);
    }
    return pBlock;
  }

  void dllfree( void* pPtr )
  {
    free(pPtr);
  }

  void* dllcalloc(size_t num, size_t size)
  {
    void* pBlock = calloc(num, size);
    if (!pBlock)
    {
      CLog::Log(LOGSEVERE, "calloc %" PRIdS" bytes failed, crash imminent", size);
    }
    return pBlock;
  }

  void* dllrealloc( void *memblock, size_t size )
  {
    void* pBlock =  realloc(memblock, size);
    if (!pBlock)
    {
      CLog::Log(LOGSEVERE, "realloc %" PRIdS" bytes failed, crash imminent", size);
    }
    return pBlock;
  }

  void dllexit(int iCode)
  {
    not_implement("msvcrt.dll fake function exit() called\n");      //warning
  }

  void dllabort()
  {
    not_implement("msvcrt.dll fake function abort() called\n");     //warning
  }

  void* dll__dllonexit(PFV input, PFV ** start, PFV ** end)
  {
    //ported from WINE code
    PFV *tmp;
    int len;

    if (!start || !*start || !end || !*end)
    {
      //FIXME("bad table\n");
      return NULL;
    }

    len = (*end - *start);

    if (++len <= 0)
      return NULL;

    tmp = (PFV*) realloc (*start, len * sizeof(tmp) );
    if (!tmp)
      return NULL;
    *start = tmp;
    *end = tmp + len;
    tmp[len - 1] = input;
    return (void *)input;

    //wrong handling, this function is used for register functions
    //that called before exit use _initterm functions.

    //dllReleaseAll( );
    //return TRUE;
  }

  _onexit_t dll_onexit(_onexit_t func)
  {
    not_implement("msvcrt.dll fake function dll_onexit() called\n");

    // register to dll unload list
    // return func if succsesfully added to the dll unload list
    return NULL;
  }

  int dllputs(const char* szLine)
  {
    if (!szLine[0]) return EOF;
    if (szLine[strlen(szLine) - 1] != '\n')
      CLog::Log(LOGDEBUG,"  msg: %s", szLine);
    else
      CLog::Log(LOGDEBUG,"  msg: %s\n", szLine);

    // return a non negative value
    return 0;
  }

  int dllprintf(const char *format, ...)
  {
    va_list va;
    static char tmp[2048];
    va_start(va, format);
    _vsnprintf(tmp, 2048, format, va);
    va_end(va);
    tmp[2048 - 1] = 0;
    CLog::Log(LOGDEBUG, "  msg: %s", tmp);

    return strlen(tmp);
  }

  char *dll_fullpath(char *absPath, const char *relPath, size_t maxLength)
  {
    unsigned int len = strlen(relPath);
    if (len > maxLength && absPath != NULL) return NULL;

    // dll has to make sure it uses the correct path for now
    if (len > 1 && relPath[1] == ':')
    {
      if (absPath == NULL) absPath = dll_strdup(relPath);
      else
      {
        strncpy(absPath, relPath, maxLength);
        if (maxLength != 0)
          absPath[maxLength-1] = '\0';
      }
      return absPath;
    }
    if (!strncmp(relPath, "\\Device\\Cdrom0", 14))
    {
      // needed?
      if (absPath == NULL) absPath = strdup(relPath);
      else
      {
        strncpy(absPath, relPath, maxLength);
        if (maxLength != 0)
          absPath[maxLength-1] = '\0';
      }
      return absPath;
    }

    not_implement("msvcrt.dll incomplete function _fullpath(...) called\n");      //warning
    return NULL;
  }

  FILE* dll_popen(const char *command, const char *mode)
  {
    not_implement("msvcrt.dll fake function _popen(...) called\n"); //warning
    return NULL;
  }

  void *dll_dlopen(const char *filename, int flag)
  {
#if defined(TARGET_ANDROID)
    CAndroidDyload temp;
    return temp.Open(filename);
#elif !defined(TARGET_WINDOWS)
    return dlopen(filename, flag);
#else
    return NULL;
#endif
  }

  int dll_pclose(FILE *stream)
  {
    not_implement("msvcrt.dll fake function _pclose(...) called\n");        //warning
    return 0;
  }

  FILE* dll_fdopen(int fd, const char* mode)
  {
    EmuFileObject* o = g_emuFileWrapper.GetFileObjectByDescriptor(fd);
    if (o)
    {
      if(!o->used)
        return NULL;

      int nmode = convert_fmode(mode);
      if( (o->mode & nmode) != nmode)
        CLog::Log(LOGWARNING, "dll_fdopen - mode 0x%x differs from fd mode 0x%x", nmode, o->mode);
      return &o->file_emu;
    }
    else if (!IS_STD_DESCRIPTOR(fd))
    {
      // it might be something else than a file, or the file is not emulated
      // let the operating system handle it
      return _fdopen(fd, mode);
    }

    not_implement("msvcrt.dll incomplete function _fdopen(...) called\n");
    return NULL;
  }

  int dll_open(const char* szFileName, int iMode)
  {
    char str[1024];
    int size = sizeof(str);
    // move to CFile classes
    if (strncmp(szFileName, "\\Device\\Cdrom0", 14) == 0)
    {
      // replace "\\Device\\Cdrom0" with "D:"
      strncpy(str, "D:", size);
      if (size)
      {
        str[size-1] = '\0';
        strncat(str, szFileName + 14, size - strlen(str));
      }
    }
    else
    {
      strncpy(str, szFileName, size);
      if (size)
        str[size-1] = '\0';
    }

    CFile* pFile = new CFile();
    bool bWrite = false;
    if ((iMode & O_RDWR) || (iMode & O_WRONLY))
      bWrite = true;
    bool bOverwrite=false;
    if ((iMode & _O_TRUNC) || (iMode & O_CREAT))
      bOverwrite = true;
    // currently always overwrites
    bool bResult;

    // We need to validate the path here as some calls from ie. libdvdnav
    // or the python DLLs have malformed slashes on Win32
    // (-> E:\test\VIDEO_TS/VIDEO_TS.BUP))
    if (bWrite)
      bResult = pFile->OpenForWrite(CUtil::ValidatePath(str), bOverwrite);
    else
      bResult = pFile->Open(CUtil::ValidatePath(str), READ_TRUNCATED);

    if (bResult)
    {
      EmuFileObject* object = g_emuFileWrapper.RegisterFileObject(pFile);
      if (object == NULL)
      {
        pFile->Close();
        delete pFile;
        return -1;
      }
      object->mode = iMode;
      return g_emuFileWrapper.GetDescriptorByStream(&object->file_emu);
    }
    delete pFile;
    return -1;
  }

  FILE* dll_freopen(const char *path, const char *mode, FILE *stream)
  {
    if (g_emuFileWrapper.StreamIsEmulatedFile(stream))
    {
      dll_fclose(stream);
      return dll_fopen(path, mode);
    }
    else if (!IS_STD_STREAM(stream))
    {
      // Translate the path
      return freopen(CSpecialProtocol::TranslatePath(path).c_str(), mode, stream);
    }

    // error
    // close stream and return NULL
    dll_fclose(stream);
    return NULL;
  }


  int dll_read(int fd, void* buffer, unsigned int uiSize)
  {
    CFile* pFile = g_emuFileWrapper.GetFileXbmcByDescriptor(fd);
    if (pFile != NULL)
    {
      errno = NOERROR;
      const ssize_t ret = pFile->Read(buffer, uiSize);
      if (ret < 0)
      {
        const int err = errno; // help compiler to optimize, "errno" can be macro
        if (err == NOERROR ||
            (err != EAGAIN && err != EINTR && err != EIO && err != EOVERFLOW && err != EWOULDBLOCK &&
             err != ECONNRESET && err != ENOTCONN && err != ETIMEDOUT &&
             err != ENOBUFS && err != ENOMEM && err != ENXIO))
          errno = EIO; // exact errno is unknown or incorrect, use default error number
        
        return -1;
      }
      return ret;
    }
    else if (!IS_STD_DESCRIPTOR(fd))
    {
      // it might be something else than a file, or the file is not emulated
      // let the operating system handle it
      return read(fd, buffer, uiSize);
    }
    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    errno = EBADF;
    return -1;
  }

  int dll_write(int fd, const void* buffer, unsigned int uiSize)
  {
    CFile* pFile = g_emuFileWrapper.GetFileXbmcByDescriptor(fd);
    if (pFile != NULL)
    {
      errno = NOERROR;
      const ssize_t ret = pFile->Write(buffer, uiSize);
      if (ret < 0)
      {
        const int err = errno; // help compiler to optimize, "errno" can be macro
        if (err == NOERROR ||
            (err != EAGAIN && err != EFBIG && err != EINTR && err != EIO && err != ENOSPC && err != EPIPE && err != EWOULDBLOCK &&
             err != ECONNRESET &&
             err != ENOBUFS && err != ENXIO &&
             err != EACCES && err != ENETDOWN && err != ENETUNREACH))
          errno = EIO; // exact errno is unknown or incorrect, use default error number

        return -1;
      }
      return ret;
    }
    else if (!IS_STD_DESCRIPTOR(fd))
    {
      // it might be something else than a file, or the file is not emulated
      // let the operating system handle it
      return write(fd, buffer, uiSize);
    }
    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    errno = EBADF;
    return -1;
  }

  int dll_fstat64(int fd, struct __stat64 *buf)
  {
    CFile* pFile = g_emuFileWrapper.GetFileXbmcByDescriptor(fd);
    if (pFile != NULL)
      return pFile->Stat(buf);
    else if (IS_STD_DESCRIPTOR(fd))
      return _fstat64(fd, buf);
    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    return -1;
  }

  int dll_close(int fd)
  {
    CFile* pFile = g_emuFileWrapper.GetFileXbmcByDescriptor(fd);
    if (pFile != NULL)
    {
      g_emuFileWrapper.UnRegisterFileObjectByDescriptor(fd);

      pFile->Close();
      delete pFile;
      return 0;
    }
    else if (!IS_STD_DESCRIPTOR(fd))
    {
      // it might be something else than a file, or the file is not emulated
      // let the operating system handle it
      return close(fd);
    }
    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    return -1;
  }

  __off64_t dll_lseeki64(int fd, __off64_t lPos, int iWhence)
  {
    CFile* pFile = g_emuFileWrapper.GetFileXbmcByDescriptor(fd);
    if (pFile != NULL)
    {
      lPos = pFile->Seek(lPos, iWhence);
      return lPos;
    }
    else if (!IS_STD_DESCRIPTOR(fd))
    {
      // it might be something else than a file, or the file is not emulated
      // let the operating system handle it
      // not supported: return lseeki64(fd, lPos, iWhence);
      CLog::Log(LOGWARNING, "msvcrt.dll: dll_lseeki64 called, TODO: add 'int64 -> long' type checking");      //warning
      return (__int64)lseek(fd, (long)lPos, iWhence);
    }
    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    return (__int64)-1;
  }

  __off_t dll_lseek(int fd, __off_t lPos, int iWhence)
  {
    if (g_emuFileWrapper.DescriptorIsEmulatedFile(fd))
    {
      return (__off_t)dll_lseeki64(fd, (__off_t)lPos, iWhence);
    }
    else if (!IS_STD_DESCRIPTOR(fd))
    {
      // it might be something else than a file, or the file is not emulated
      // let the operating system handle it
      return lseek(fd, lPos, iWhence);
    }
    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    return -1;
  }

  void dll_rewind(FILE* stream)
  {
    int fd = g_emuFileWrapper.GetDescriptorByStream(stream);
    if (fd >= 0)
    {
      dll_lseeki64(fd, 0, SEEK_SET);
    }
    else if (!IS_STD_STREAM(stream))
    {
      // it might be something else than a file, let the operating system handle it
      rewind(stream);
    }
    else
    {
    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    }
  }

  //---------------------------------------------------------------------------------------------------------
  void dll_flockfile(FILE *stream)
  {
    int fd = g_emuFileWrapper.GetDescriptorByStream(stream);
    if (fd >= 0)
    {
      g_emuFileWrapper.LockFileObjectByDescriptor(fd);
      return;
    }
    else if (!IS_STD_STREAM(stream))
    {
      // it might be something else than a file, let the operating system handle it
#ifdef TARGET_POSIX
      flockfile(stream);
      return;
#else
      CLog::Log(LOGERROR, "%s: flockfile not available on non-linux platforms",  __FUNCTION__);
#endif
    }
    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
  }

  int dll_ftrylockfile(FILE *stream)
  {
    int fd = g_emuFileWrapper.GetDescriptorByStream(stream);
    if (fd >= 0)
    {
      if (g_emuFileWrapper.TryLockFileObjectByDescriptor(fd))
        return 0;
      return -1;
    }
    else if (!IS_STD_STREAM(stream))
    {
      // it might be something else than a file, let the operating system handle it
#ifdef TARGET_POSIX
      return ftrylockfile(stream);
#else
      CLog::Log(LOGERROR, "%s: ftrylockfile not available on non-linux platforms",  __FUNCTION__);
#endif
    }
    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    return -1;
  }

  void dll_funlockfile(FILE *stream)
  {
    int fd = g_emuFileWrapper.GetDescriptorByStream(stream);
    if (fd >= 0)
    {
      g_emuFileWrapper.UnlockFileObjectByDescriptor(fd);
      return;
    }
    else if (!IS_STD_STREAM(stream))
    {
      // it might be something else than a file, let the operating system handle it
#ifdef TARGET_POSIX
      funlockfile(stream);
      return;
#else
      CLog::Log(LOGERROR, "%s: funlockfile not available on non-linux platforms",  __FUNCTION__);
#endif
    }
    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
  }

  int dll_fclose(FILE * stream)
  {
    int fd = g_emuFileWrapper.GetDescriptorByStream(stream);
    if (fd >= 0)
    {
      return dll_close(fd) == 0 ? 0 : EOF;
    }
    else if (!IS_STD_STREAM(stream))
    {
      // it might be something else than a file, let the operating system handle it
      return fclose(stream);
    }
    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    return EOF;
  }

#ifndef TARGET_POSIX
  // should be moved to CFile classes
  intptr_t dll_findfirst(const char *file, struct _finddata_t *data)
  {
    struct _finddata64i32_t data64i32;
    intptr_t ret = dll_findfirst64i32(file, &data64i32);
    if (ret != -1)
    {
      int size = sizeof(data->name);
      strncpy(data->name, data64i32.name, size);
      if (size)
        data->name[size - 1] = '\0';
      data->size = (_fsize_t)data64i32.size;
      data->time_write = (time_t)data64i32.time_write;
      data->time_access = (time_t)data64i32.time_access;
    }
    return ret;
  }

  intptr_t dll_findfirst64i32(const char *file, struct _finddata64i32_t *data)
  {
    char str[1024];
    int size = sizeof(str);
    CURL url(CSpecialProtocol::TranslatePath(file));
    if (url.IsLocal())
    {
      // move to CFile classes
      if (strncmp(file, "\\Device\\Cdrom0", 14) == 0)
      {
        // replace "\\Device\\Cdrom0" with "D:"
        strncpy(str, "D:", size);
        if (size)
        {
          str[size - 1] = '\0';
          strncat(str, file + 14, size - strlen(str));
        }
      }
      else
      {
        strncpy(str, file, size);
        if (size)
          str[size - 1] = '\0';
      }

      // Make sure the slashes are correct & translate the path
      struct _wfinddata64i32_t wdata;
      std::wstring strwfile;
      g_charsetConverter.utf8ToW(CUtil::ValidatePath(CSpecialProtocol::TranslatePath(str)), strwfile, false);
      intptr_t ret = _wfindfirst64i32(strwfile.c_str(), &wdata);
      if (ret != -1)
        to_finddata64i32(&wdata, data);
      return ret;
    }
    // non-local files. handle through IDirectory-class - only supports '*.bah' or '*.*'
    std::string strURL(file);
    std::string strMask;
    if (url.GetFileName().find("*.*") != std::string::npos)
    {
      std::string strReplaced = url.GetFileName();
      StringUtils::Replace(strReplaced, "*.*","");
      url.SetFileName(strReplaced);
    }
    else if (url.GetFileName().find("*.") != std::string::npos)
    {
      strMask = URIUtils::GetExtension(url.GetFileName());
      url.SetFileName(url.GetFileName().substr(0, url.GetFileName().find("*.")));
    }
    else if (url.GetFileName().find("*") != std::string::npos)
    {
      std::string strReplaced = url.GetFileName();
      StringUtils::Replace(strReplaced, "*","");
      url.SetFileName(strReplaced);
    }
    int iDirSlot=0; // locate next free directory
    while ((vecDirsOpen[iDirSlot].curr_index != -1) && (iDirSlot<MAX_OPEN_DIRS)) iDirSlot++;
    if (iDirSlot >= MAX_OPEN_DIRS)
      return -1; // no free slots
    if (url.IsProtocol("filereader"))
    {
      CURL url2(url.GetFileName());
      url = url2;
    }
    strURL = url.Get();
    bVecDirsInited = true;
    vecDirsOpen[iDirSlot].items.Clear();
    XFILE::CDirectory::GetDirectory(strURL, vecDirsOpen[iDirSlot].items, strMask);
    if (vecDirsOpen[iDirSlot].items.Size())
    {
      int size = sizeof(data->name);
      strncpy(data->name,vecDirsOpen[iDirSlot].items[0]->GetLabel().c_str(), size);
      if (size)
        data->name[size - 1] = '\0';
      data->size = static_cast<_fsize_t>(vecDirsOpen[iDirSlot].items[0]->m_dwSize);
      data->time_write = 0;
      data->time_access = 0;
      vecDirsOpen[iDirSlot].curr_index = 0;
      return (intptr_t)&vecDirsOpen[iDirSlot];
    }
    vecDirsOpen[iDirSlot].curr_index = -1;
    return -1; // whatever != NULL
  }

  // should be moved to CFile classes
  int dll_findnext(intptr_t f, _finddata_t* data)
  {
    struct _finddata64i32_t data64i32;
    int ret = dll_findnext64i32(f, &data64i32);
    if (ret == 0)
    {
      int size = sizeof(data->name);
      strncpy(data->name, data64i32.name, size);
      if (size)
        data->name[size - 1] = '\0';
      data->size = (_fsize_t)data64i32.size;
      data->time_write = (time_t)data64i32.time_write;
      data->time_access = (time_t)data64i32.time_access;
    }
    return ret;
  }

  int dll_findnext64i32(intptr_t f, _finddata64i32_t* data)
  {
    int found = MAX_OPEN_DIRS;
    for (int i = 0; i < MAX_OPEN_DIRS; i++)
    {
      if (f == (intptr_t)&vecDirsOpen[i] && vecDirsOpen[i].curr_index != -1)
      {
        found = i;
        break;
      }
    }
    if (found >= MAX_OPEN_DIRS)
    {
      struct _wfinddata64i32_t wdata;
      to_wfinddata64i32(data, &wdata);
      intptr_t ret = _wfindnext64i32(f, &wdata); // local dir
      if (ret != -1)
        to_finddata64i32(&wdata, data);
      return ret;
    }

    // we have a valid data struture. get next item!
    int iItem = vecDirsOpen[found].curr_index;
    if (iItem+1 < vecDirsOpen[found].items.Size()) // we have a winner!
    {
      int size = sizeof(data->name);
      strncpy(data->name,vecDirsOpen[found].items[iItem+1]->GetLabel().c_str(), size);
      if (size)
        data->name[size - 1] = '\0';
      data->size = static_cast<_fsize_t>(vecDirsOpen[found].items[iItem+1]->m_dwSize);
      vecDirsOpen[found].curr_index++;
      return 0;
    }

    vecDirsOpen[found].items.Clear();
    return -1;
  }

  int dll_findclose(intptr_t handle)
  {
    int found = MAX_OPEN_DIRS;
    for (int i = 0; i < MAX_OPEN_DIRS; i++)
    {
      if (handle == (intptr_t)&vecDirsOpen[i] && vecDirsOpen[i].curr_index != -1)
      {
        found = i;
        break;
      }
    }
    if (found >= MAX_OPEN_DIRS)
      return _findclose(handle);

    vecDirsOpen[found].items.Clear();
    vecDirsOpen[found].curr_index = -1;
    return 0;
  }

  void dll__security_error_handler(int code, void *data)
  {
    //NOTE: __security_error_handler has been removed in VS2005 and up
    CLog::Log(LOGERROR, "security_error, code %i", code);
  }

#endif

  DIR *dll_opendir(const char *file)
  {
    CURL url(CSpecialProtocol::TranslatePath(file));
    if (url.IsLocal())
    { // Make sure the slashes are correct & translate the path
      return opendir(CUtil::ValidatePath(url.Get().c_str()).c_str());
    }

    // locate next free directory
    int iDirSlot=0;
    while ((iDirSlot<MAX_OPEN_DIRS) && (vecDirsOpen[iDirSlot].curr_index != -1)) iDirSlot++;
    if (iDirSlot >= MAX_OPEN_DIRS)
    {
      CLog::Log(LOGDEBUG, "Dll: Max open dirs reached");
      return NULL; // no free slots
    }

    if (url.IsProtocol("filereader"))
    {
      CURL url2(url.GetFileName());
      url = url2;
    }

    bVecDirsInited = true;
    vecDirsOpen[iDirSlot].items.Clear();

    if (XFILE::CDirectory::GetDirectory(url.Get(), vecDirsOpen[iDirSlot].items))
    {
      vecDirsOpen[iDirSlot].curr_index = 0;
      return (DIR *)&vecDirsOpen[iDirSlot];
    }
    else
      return NULL;
  }

  struct dirent *dll_readdir(DIR *dirp)
  {
    if (!dirp)
      return NULL;

    bool emulated(false);
    for (int i = 0; i < MAX_OPEN_DIRS; i++)
    {
      if (dirp == (DIR*)&vecDirsOpen[i])
      {
        emulated = true;
        break;
      }
    }
    if (!emulated)
      return readdir(dirp); // local dir

    // dirp is actually a SDirData*
    SDirData* dirData = (SDirData*)dirp;
    if (dirData->last_entry)
      free(dirData->last_entry);
    struct dirent *entry = NULL;
    entry = (dirent*) malloc(sizeof(*entry));
    if (dirData->curr_index < dirData->items.Size() + 2)
    { // simulate the '.' and '..' dir entries
      if (dirData->curr_index == 0)
        strncpy(entry->d_name, ".\0", 2);
      else if (dirData->curr_index == 1)
        strncpy(entry->d_name, "..\0", 3);
      else
      {
        strncpy(entry->d_name, dirData->items[dirData->curr_index - 2]->GetLabel().c_str(), sizeof(entry->d_name));
        entry->d_name[sizeof(entry->d_name)-1] = '\0'; // null-terminate any truncated paths
      }
      dirData->last_entry = entry;
      dirData->curr_index++;
      return entry;
    }
    free(entry);
    return NULL;
  }

  int dll_closedir(DIR *dirp)
  {
    bool emulated(false);
    for (int i = 0; i < MAX_OPEN_DIRS; i++)
    {
      if (dirp == (DIR*)&vecDirsOpen[i])
      {
        emulated = true;
        break;
      }
    }
    if (!emulated)
      return closedir(dirp);

    SDirData* dirData = (SDirData*)dirp;
    dirData->items.Clear();
    if (dirData->last_entry)
    {
      dirData->last_entry = NULL;
    }
    dirData->curr_index = -1;
    return 0;
  }

  void dll_rewinddir(DIR *dirp)
  {
    bool emulated(false);
    for (int i = 0; i < MAX_OPEN_DIRS; i++)
    {
      if (dirp == (DIR*)&vecDirsOpen[i])
      {
        emulated = true;
        break;
      }
    }
    if (!emulated)
    {
      rewinddir(dirp);
      return;
    }

    SDirData* dirData = (SDirData*)dirp;
    if (dirData->last_entry)
    {
      dirData->last_entry = NULL;
    }
    dirData->curr_index = 0;
  }

  char* dll_fgets(char* pszString, int num ,FILE * stream)
  {
    CFile* pFile = g_emuFileWrapper.GetFileXbmcByStream(stream);
    if (pFile != NULL)
    {
      if (pFile->GetPosition() < pFile->GetLength())
      {
        bool bRead = pFile->ReadString(pszString, num);
        if (bRead)
        {
          return pszString;
        }
      }
      else return NULL; //eof
    }
    else if (!IS_STD_STREAM(stream))
    {
      // it might be something else than a file, or the file is not emulated
      // let the operating system handle it
      return fgets(pszString, num, stream);
    }
    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    return NULL;
  }

  int dll_feof(FILE * stream)
  {
    CFile* pFile = g_emuFileWrapper.GetFileXbmcByStream(stream);
    if (pFile != NULL)
    {
      if (pFile->GetPosition() < pFile->GetLength()) return 0;
      else return 1;
    }
    else if (!IS_STD_STREAM(stream))
    {
      // it might be something else than a file, or the file is not emulated
      // let the operating system handle it
      return feof(stream);
    }
    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    return 1; // eof by default
  }

  int dll_fread(void * buffer, size_t size, size_t count, FILE * stream)
  {
    if (size == 0 || count == 0)
      return 0;

    CFile* pFile = g_emuFileWrapper.GetFileXbmcByStream(stream);
    if (pFile != NULL)
    {
      size_t read = 0;
      const size_t bufSize = size * count;
      do // fread() must read all data until buffer is filled or eof/error occurs
      {
        const ssize_t r = pFile->Read(((int8_t*)buffer) + read, bufSize - read);
        if (r <= 0)
          break;
        read += r;
      } while (bufSize > read);
      return read / size;
    }
    else if (!IS_STD_STREAM(stream))
    {
      // it might be something else than a file, let the operating system handle it
      return fread(buffer, size, count, stream);
    }
    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    return 0;
  }

  int dll_fgetc(FILE* stream)
  {
    if (g_emuFileWrapper.StreamIsEmulatedFile(stream))
    {
      // it is a emulated file
      unsigned char buf;

      if (dll_fread(&buf, 1, 1, stream) <= 0)
        return EOF;

      return (int)buf;
    }
    else if (!IS_STD_STREAM(stream))
    {
      // it might be something else than a file, or the file is not emulated
      // let the operating system handle it
      return getc(stream);
    }
    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    return EOF;
  }

  int dll_getc(FILE* stream)
  {
    if (g_emuFileWrapper.StreamIsEmulatedFile(stream))
    {
      // This routine is normally implemented as a macro with the same result as fgetc().
      return dll_fgetc(stream);
    }
    else if (!IS_STD_STREAM(stream))
    {
      // it might be something else than a file, or the file is not emulated
      // let the operating system handle it
      return getc(stream);
    }
    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    return EOF;
  }

  FILE* dll_fopen(const char* filename, const char* mode)
  {
    FILE* file = NULL;
#if defined(TARGET_LINUX) && !defined(TARGET_ANDROID)
    if (strcmp(filename, MOUNTED) == 0
    ||  strcmp(filename, MNTTAB) == 0)
    {
      CLog::Log(LOGINFO, "%s - something opened the mount file, let's hope it knows what it's doing", __FUNCTION__);
      return fopen(filename, mode);
    }
#endif
    int fd = dll_open(filename, convert_fmode(mode));
    if (fd >= 0)
    {
      file = g_emuFileWrapper.GetStreamByDescriptor(fd);
    }

    return file;
  }

  int dll_fopen_s(FILE** pFile, const char * filename, const char * mode)
  {
    if (pFile == NULL || filename == NULL || mode == NULL)
      return EINVAL;

    *pFile = dll_fopen(filename, mode);
    if (*pFile == NULL)
      return errno;

    return 0;
  }

  int dll_putc(int c, FILE *stream)
  {
    if (g_emuFileWrapper.StreamIsEmulatedFile(stream) || IS_STD_STREAM(stream))
    {
      return dll_fputc(c, stream);
    }
    else
    {
      return putc(c, stream);
    }
    return EOF;
  }

  int dll_putchar(int c)
  {
    return dll_putc(c, stdout);
  }

  int dll_fputc(int character, FILE* stream)
  {
    if (IS_STDOUT_STREAM(stream) || IS_STDERR_STREAM(stream))
    {
      unsigned char tmp[2] = { (unsigned char)character, 0 };
      dllputs((char *)tmp);
      return character;
    }
    else
    {
      if (g_emuFileWrapper.StreamIsEmulatedFile(stream))
      {
        int fd = g_emuFileWrapper.GetDescriptorByStream(stream);
        if (fd >= 0)
        {
          unsigned char c = (unsigned char)character;
          int iItemsWritten = dll_write(fd, &c, 1);
          if (iItemsWritten == 1)
            return character;
        }
      }
      else if (!IS_STD_STREAM(stream))
      {
        // it might be something else than a file, or the file is not emulated
        // let the operating system handle it
        return fputc(character, stream);
      }
    }
    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    return EOF;
  }

  int dll_fputs(const char * szLine, FILE* stream)
  {
    if (IS_STDOUT_STREAM(stream) || IS_STDERR_STREAM(stream))
    {
      dllputs(szLine);
      return 0;
    }
    else
    {
      if (g_emuFileWrapper.StreamIsEmulatedFile(stream))
      {
        size_t len = strlen(szLine);
        return dll_fwrite(static_cast<const void*>(szLine), sizeof(char), len, stream);
      }
      else if (!IS_STD_STREAM(stream))
      {
        // it might be something else than a file, or the file is not emulated
        // let the operating system handle it
        return fputs(szLine, stream);
      }
    }

    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    return EOF;
  }

  int dll_fseek64(FILE* stream, off64_t offset, int origin)
  {
    int fd = g_emuFileWrapper.GetDescriptorByStream(stream);
    if (fd >= 0)
    {
      if (dll_lseeki64(fd, offset, origin) != -1)
      {
        return 0;
      }
      else return -1;
    }
    else if (!IS_STD_STREAM(stream))
    {
      // it might be something else than a file, or the file is not emulated
      // let the operating system handle it
#if defined(TARGET_DARWIN) || defined(TARGET_FREEBSD) || defined(TARGET_ANDROID)
      return fseek(stream, offset, origin);
#else
      return fseeko64(stream, offset, origin);
#endif
    }
    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    return -1;
  }

  int dll_fseek(FILE *stream, long offset, int origin)
  {
    return dll_fseek64(stream, offset, origin);
  }

  int dll_ungetc(int c, FILE* stream)
  {
    if (g_emuFileWrapper.StreamIsEmulatedFile(stream))
    {
      // it is a emulated file
      int d;
      if (dll_fseek(stream, -1, SEEK_CUR)!=0)
        return EOF;
      d = dll_fgetc(stream);
      if (d == EOF)
        return EOF;

      dll_fseek(stream, -1, SEEK_CUR);
      if (c != d)
      {
        CLog::Log(LOGWARNING, "%s: c != d",  __FUNCTION__);
        d = fputc(c, stream);
        if (d != c)
          CLog::Log(LOGERROR, "%s: Write failed!",  __FUNCTION__);
        else
          dll_fseek(stream, -1, SEEK_CUR);
      }
      return d;
    }
    else if (!IS_STD_STREAM(stream))
    {
      // it might be something else than a file, or the file is not emulated
      // let the operating system handle it
      return ungetc(c, stream);
    }
    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    return EOF;
  }

  long dll_ftell(FILE *stream)
  {
    return (long)dll_ftell64(stream);
  }

  off64_t dll_ftell64(FILE *stream)
  {
    CFile* pFile = g_emuFileWrapper.GetFileXbmcByStream(stream);
    if (pFile != NULL)
    {
       return (off64_t)pFile->GetPosition();
    }
    else if (!IS_STD_STREAM(stream))
    {
      // it might be something else than a file, or the file is not emulated
      // let the operating system handle it
#if defined(TARGET_DARWIN) || defined(TARGET_FREEBSD) || defined(TARGET_ANDROID)
      return ftello(stream);
#else
      return ftello64(stream);
#endif
    }
    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    return -1;
  }

  long dll_tell(int fd)
  {
    CFile* pFile = g_emuFileWrapper.GetFileXbmcByDescriptor(fd);
    if (pFile != NULL)
    {
       return (long)pFile->GetPosition();
    }
    else if (!IS_STD_DESCRIPTOR(fd))
    {
      // it might be something else than a file, or the file is not emulated
      // let the operating system handle it
#ifndef TARGET_POSIX
      return tell(fd);
#else
      return lseek(fd, 0, SEEK_CUR);
#endif
    }
    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    return -1;
  }

  __int64 dll_telli64(int fd)
  {
    CFile* pFile = g_emuFileWrapper.GetFileXbmcByDescriptor(fd);
    if (pFile != NULL)
    {
       return (__int64)pFile->GetPosition();
    }
    else if (!IS_STD_DESCRIPTOR(fd))
    {
      // it might be something else than a file, or the file is not emulated
      // let the operating system handle it
      // not supported return telli64(fd);
      CLog::Log(LOGWARNING, "msvcrt.dll: dll_telli64 called, TODO: add 'int64 -> long' type checking");      //warning
#ifndef TARGET_POSIX
      return (__int64)tell(fd);
#elif defined(TARGET_DARWIN) || defined(TARGET_FREEBSD) || defined(TARGET_ANDROID)
      return lseek(fd, 0, SEEK_CUR);
#else
      return lseek64(fd, 0, SEEK_CUR);
#endif
    }
    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    return -1;
  }

  size_t dll_fwrite(const void * buffer, size_t size, size_t count, FILE* stream)
  {
    if (size == 0 || count == 0)
      return 0;

    if (IS_STDOUT_STREAM(stream) || IS_STDERR_STREAM(stream))
    {
      char* buf = (char*)malloc(size * count + 1);
      if (buf)
      {
        memcpy(buf, buffer, size * count);
        buf[size * count] = 0; // string termination

        CLog::Log(LOGDEBUG, "%s", buf);

        free(buf);
        return count;
      }
    }
    else
    {
      CFile* pFile = g_emuFileWrapper.GetFileXbmcByStream(stream);
      if (pFile != NULL)
      {
        size_t written = 0;
        const size_t bufSize = size * count;
        do // fwrite() must write all data until whole buffer is written or error occurs
        {
          const ssize_t w = pFile->Write(((int8_t*)buffer) + written, bufSize - written);
          if (w <= 0)
            break;
          written += w;
        } while (bufSize > written);
        return written / size;
      }
      else if (!IS_STD_STREAM(stream))
      {
        // it might be something else than a file, or the file is not emulated
        // let the operating system handle it
        return fwrite(buffer, size, count, stream);
      }
    }
    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    return 0;
  }

  int dll_fflush(FILE* stream)
  {
    CFile* pFile = g_emuFileWrapper.GetFileXbmcByStream(stream);
    if (pFile != NULL)
    {
      pFile->Flush();
      return 0;
    }
    else if (!IS_STD_STREAM(stream))
    {
      // it might be something else than a file, or the file is not emulated
      // let the operating system handle it
      return fflush(stream);
    }

    // std stream, no need to flush
    return 0;
  }

  int dll_ferror(FILE* stream)
  {
    CFile* pFile = g_emuFileWrapper.GetFileXbmcByStream(stream);
    if (pFile != NULL)
    {
      // unimplemented
      return 0;
    }
    else if (IS_STD_STREAM(stream))
      return 0;
    else
      return ferror(stream);
  }

  int dllvprintf(const char *format, va_list va)
  {
    std::string buffer = StringUtils::FormatV(format, va);
    CLog::Log(LOGDEBUG, "  msg: %s", buffer.c_str());
    return buffer.length();
  }

  int dll_vfprintf(FILE *stream, const char *format, va_list va)
  {
    static char tmp[2048];

    if (_vsnprintf(tmp, 2048, format, va) == -1)
    {
      CLog::Log(LOGWARNING, "dll_vfprintf: Data lost due to undersized buffer");
    }
    tmp[2048 - 1] = 0;

    if (IS_STDOUT_STREAM(stream) || IS_STDERR_STREAM(stream))
    {
      CLog::Log(LOGINFO, "  msg: %s", tmp);
      return strlen(tmp);
    }
    else
    {
      CFile* pFile = g_emuFileWrapper.GetFileXbmcByStream(stream);
      if (pFile != NULL)
      {
        int len = strlen(tmp);
        // replace all '\n' occurences with '\r\n'...
        char tmp2[2048];
        int j = 0;
        for (int i = 0; i < len; i++)
        {
          if (j == 2047)
          { // out of space
            if (i != len-1)
              CLog::Log(LOGWARNING, "dll_fprintf: Data lost due to undersized buffer");
            break;
          }
          if (tmp[i] == '\n' && ((i > 0 && tmp[i-1] != '\r') || i == 0) && j < 2047 - 2)
          { // need to add a \r
            tmp2[j++] = '\r';
            tmp2[j++] = '\n';
          }
          else
          { // just add the character as-is
            tmp2[j++] = tmp[i];
          }
        }
        // terminate string
        tmp2[j] = 0;
        len = strlen(tmp2);
        pFile->Write(tmp2, len);
        return len;
      }
      else if (!IS_STD_STREAM(stream) && IS_VALID_STREAM(stream))
      {
        // it might be something else than a file, or the file is not emulated
        // let the operating system handle it
        return vfprintf(stream, format, va);
      }
    }

    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    return strlen(tmp);
  }

  int dll_fscanf(FILE* stream, const char* format, ...)
  {
    CLog::Log(LOGERROR, "%s is not implemented",  __FUNCTION__);
    return -1;
  }

  int dll_fprintf(FILE* stream, const char* format, ...)
  {
    int res;
    va_list va;
    va_start(va, format);
    res = dll_vfprintf(stream, format, va);
    va_end(va);
    return res;
  }

  int dll_fgetpos(FILE* stream, fpos_t* pos)
  {
    fpos64_t tmpPos;
    int ret;

    ret = dll_fgetpos64(stream, &tmpPos);
#if !defined(TARGET_POSIX) || defined(TARGET_DARWIN) || defined(TARGET_FREEBSD) || defined(TARGET_ANDROID)
    *pos = (fpos_t)tmpPos;
#else
    pos->__pos = (off_t)tmpPos.__pos;
#endif
    return ret;
  }

  int dll_fgetpos64(FILE *stream, fpos64_t *pos)
  {
    CFile* pFile = g_emuFileWrapper.GetFileXbmcByStream(stream);
    if (pFile != NULL)
    {
#if !defined(TARGET_POSIX) || defined(TARGET_DARWIN) || defined(TARGET_FREEBSD) || defined(TARGET_ANDROID)
      *pos = pFile->GetPosition();
#else
      pos->__pos = pFile->GetPosition();
#endif
      return 0;
    }
    else if (!IS_STD_STREAM(stream))
    {
      // it might be something else than a file, or the file is not emulated
      // let the operating system handle it
      return fgetpos(stream, pos);
    }
    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    return EINVAL;
  }

  int dll_fsetpos64(FILE* stream, const fpos64_t* pos)
  {
    int fd = g_emuFileWrapper.GetDescriptorByStream(stream);
    if (fd >= 0)
    {
#if !defined(TARGET_POSIX) || defined(TARGET_DARWIN) || defined(TARGET_FREEBSD) || defined(TARGET_ANDROID)
      if (dll_lseeki64(fd, *pos, SEEK_SET) >= 0)
#else
      if (dll_lseeki64(fd, (__off64_t)pos->__pos, SEEK_SET) >= 0)
#endif
      {
        return 0;
      }
      else
      {
        return EINVAL;
      }
    }
    else if (!IS_STD_STREAM(stream))
    {
      // it might be something else than a file, or the file is not emulated
      // let the operating system handle it
#if !defined(TARGET_POSIX) || defined(TARGET_DARWIN) || defined(TARGET_FREEBSD) || defined(TARGET_ANDROID)
      return fsetpos(stream, pos);
#else
      return fsetpos64(stream, pos);
#endif
    }
    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    return EINVAL;
  }

  int dll_fsetpos(FILE* stream, const fpos_t* pos)
  {
    int fd = g_emuFileWrapper.GetDescriptorByStream(stream);
    if (fd >= 0)
    {
      fpos64_t tmpPos;
#if !defined(TARGET_POSIX) || defined(TARGET_DARWIN) || defined(TARGET_FREEBSD) || defined(TARGET_ANDROID)
      tmpPos= *pos;
#else
      tmpPos.__pos = (off64_t)(pos->__pos);
#endif
      return dll_fsetpos64(stream, &tmpPos);
    }
    else if (!IS_STD_STREAM(stream))
    {
      // it might be something else than a file, or the file is not emulated
      // let the operating system handle it
      return fsetpos(stream, (fpos_t*)pos);
    }
    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    return EINVAL;
  }

  int dll_fileno(FILE* stream)
  {
    int fd = g_emuFileWrapper.GetDescriptorByStream(stream);
    if (fd >= 0)
    {
      return fd;
    }
    else if (IS_STDIN_STREAM(stream))
    {
      return 0;
    }
    else if (IS_STDOUT_STREAM(stream))
    {
      return 1;
    }
    else if (IS_STDERR_STREAM(stream))
    {
      return 2;
    }
    else
    {
      return fileno(stream);
    }

    return -1;
  }

  void dll_clearerr(FILE* stream)
  {
    if (g_emuFileWrapper.StreamIsEmulatedFile(stream))
    {
      // not implemented
    }
    else if (!IS_STD_STREAM(stream))
    {
      clearerr(stream);
    }
  }

  char* dll_strdup( const char* str)
  {
    char* pdup;
    pdup = strdup(str);
    return pdup;
  }


  //Critical Section has been fixed in EMUkernel32.cpp

  int dll_initterm(PFV * start, PFV * end)        //pncrt.dll
  {
    PFV * temp;
    for (temp = start; temp < end; temp ++)
      if (*temp)
        (*temp)(); //call initial function table.
    return 0;
  }

  //SLOW CODE SHOULD BE REVISED
  int dll_stat(const char *path, struct stat *buffer)
  {
    if (!strnicmp(path, "shout://", 8)) // don't stat shoutcast
      return -1;
    if (!strnicmp(path, "http://", 7)
    ||  !strnicmp(path, "https://", 8)) // don't stat http
      return -1;
    if (!strnicmp(path, "mms://", 6)) // don't stat mms
      return -1;

#ifdef TARGET_POSIX
    if (!_stricmp(path, "D:") || !_stricmp(path, "D:\\"))
    {
      buffer->st_mode = S_IFDIR;
      return 0;
    }
#endif
    if (!stricmp(path, "\\Device\\Cdrom0") || !stricmp(path, "\\Device\\Cdrom0\\"))
    {
      buffer->st_mode = _S_IFDIR;
      return 0;
    }

    struct __stat64 tStat;
    if (CFile::Stat(path, &tStat) == 0)
    {
      CUtil::Stat64ToStat(buffer, &tStat);
      return 0;
    }
    // errno is set by file.Stat(...)
    return -1;
  }

  int dll_stati64(const char *path, struct _stati64 *buffer)
  {
    struct __stat64 a;
    memset(&a, 0, sizeof(a));

    if(dll_stat64(path, &a) == 0)
    {
      CUtil::Stat64ToStatI64(buffer, &a);
      return 0;
    }
    return -1;
  }

  int dll_stat64(const char *path, struct __stat64 *buffer)
  {
    if (!strnicmp(path, "shout://", 8)) // don't stat shoutcast
      return -1;
    if (!strnicmp(path, "http://", 7)
    ||  !strnicmp(path, "https://", 8)) // don't stat http
      return -1;
    if (!strnicmp(path, "mms://", 6)) // don't stat mms
      return -1;

#ifdef TARGET_POSIX
    if (!_stricmp(path, "D:") || !_stricmp(path, "D:\\"))
    {
      buffer->st_mode = _S_IFDIR;
      return 0;
    }
#endif
    if (!stricmp(path, "\\Device\\Cdrom0") || !stricmp(path, "\\Device\\Cdrom0\\"))
    {
      buffer->st_mode = _S_IFDIR;
      return 0;
    }

    return CFile::Stat(path, buffer);
  }

#ifdef TARGET_WINDOWS
  int dll_stat64i32(const char *path, struct _stat64i32 *buffer)
  {
    struct __stat64 a;
    if(dll_stat64(path, &a) == 0)
    {
      CUtil::Stat64ToStat64i32(buffer, &a);
      return 0;
    }
    return -1;
  }
#endif

  int dll_fstat(int fd, struct stat* buffer)
  {
    CFile* pFile = g_emuFileWrapper.GetFileXbmcByDescriptor(fd);
    if (pFile != NULL)
    {
      struct __stat64 tStat;
      if (pFile->Stat(&tStat) == 0)
      {
        CUtil::Stat64ToStat(buffer, &tStat);
        return 0;
      }
    }
    else if (!IS_STD_DESCRIPTOR(fd))
    {
      return fstat(fd, buffer);
    }

    // fstat on stdin, stdout or stderr should fail
    // this is what python expects
    return -1;
  }

  int dll_fstati64(int fd, struct _stati64 *buffer)
  {
    CFile* pFile = g_emuFileWrapper.GetFileXbmcByDescriptor(fd);
    if (pFile != NULL)
    {
      CLog::Log(LOGINFO, "Stating open file");

      buffer->st_size = pFile->GetLength();
      buffer->st_mode = _S_IFREG;
      return 0;
    }
    else if (!IS_STD_DESCRIPTOR(fd))
    {
      CLog::Log(LOGWARNING, "msvcrt.dll: dll_fstati64 called, TODO: add 'int64 <-> long' type checking");      //warning
      // need to use fstat and convert everything
      struct stat temp;
      int res = fstat(fd, &temp);
      if (res == 0)
      {
        CUtil::StatToStatI64(buffer, &temp);
      }
      return res;
    }

    // fstat on stdin, stdout or stderr should fail
    // this is what python expects
    return -1;
  }

#ifdef TARGET_WINDOWS
  int dll_fstat64i32(int fd, struct _stat64i32 *buffer)
  {
    CFile* pFile = g_emuFileWrapper.GetFileXbmcByDescriptor(fd);
    if (pFile != NULL)
    {
      struct __stat64 tStat = {};
      if (pFile->Stat(&tStat) == 0)
      {
        CUtil::Stat64ToStat64i32(buffer, &tStat);
        return 0;
      }
      return -1;
    }
    else if (!IS_STD_DESCRIPTOR(fd))
    {
      CLog::Log(LOGWARNING, "msvcrt.dll: dll_fstati64 called, TODO: add 'int64 <-> long' type checking");      //warning
      // need to use fstat and convert everything
      struct __stat64 temp;
      int res = _fstat64(fd, &temp);
      if (res == 0)
      {
        CUtil::Stat64ToStat64i32(buffer, &temp);
      }
      return res;
    }

    // fstat on stdin, stdout or stderr should fail
    // this is what python expects
    return -1;
  }
#endif

  int dll_setmode ( int handle, int mode )
  {
    not_implement("msvcrt.dll fake function dll_setmode() called\n");
    return -1;
  }

  void dllperror(const char* s)
  {
    if (s)
    {
      CLog::Log(LOGERROR, "perror: %s", s);
    }
  }

  char* dllstrerror(int iErr)
  {
    static char szError[32];
    sprintf(szError, "err:%i", iErr);
    return (char*)szError;
  }

  int dll_mkdir(const char* dir)
  {
    if (!dir) return -1;

    // Make sure the slashes are correct & translate the path
    std::string strPath = CUtil::ValidatePath(CSpecialProtocol::TranslatePath(dir));
#ifndef TARGET_POSIX
    std::wstring strWPath;
    g_charsetConverter.utf8ToW(strPath, strWPath, false);
    return _wmkdir(strWPath.c_str());
#else
    return mkdir(strPath.c_str(), 0755);
#endif
  }

  char* dll_getcwd(char *buffer, int maxlen)
  {
    not_implement("msvcrt.dll fake function dll_getcwd() called\n");
    return (char*)"special://xbmc/";
  }

  int dll_putenv(const char* envstring)
  {
    bool added = false;

    if (envstring != NULL)
    {
      const char *value_start = strchr(envstring, '=');

      if (value_start != NULL)
      {
        char var[64];
        int size = strlen(envstring) + 1;
        char *value = (char*)malloc(size);

        if (!value)
          return -1;
        value[0] = 0;

        memcpy(var, envstring, value_start - envstring);
        var[value_start - envstring] = 0;
        char* temp = var;
        while (*temp)
        {
          *temp = (char)toupper(*temp);
          temp++;
        }

        strncpy(value, value_start + 1, size);
        if (size)
          value[size - 1] = '\0';

        {
          CSingleLock lock(dll_cs_environ);

          char** free_position = NULL;
          for (int i = 0; i < EMU_MAX_ENVIRONMENT_ITEMS && free_position == NULL; i++)
          {
            if (dll__environ[i] != NULL)
            {
              // we only support overwriting the old values
              if (strnicmp(dll__environ[i], var, strlen(var)) == 0)
              {
                // free it first
                free(dll__environ[i]);
                dll__environ[i] = NULL;
                free_position = &dll__environ[i];
              }
            }
            else
            {
              free_position = &dll__environ[i];
            }
          }

          if (free_position != NULL)
          {
            // free position, copy value
            size = strlen(var) + strlen(value) + 2;
            *free_position = (char*)malloc(size); // for '=' and 0 termination
            if ((*free_position))
            {
              strncpy(*free_position, var, size);
              (*free_position)[size - 1] = '\0';
              strncat(*free_position, "=", size - strlen(*free_position));
              strncat(*free_position, value, size - strlen(*free_position));
              added = true;
            }
          }

        }

        free(value);
      }
    }

    return added ? 0 : -1;
  }



  char* dll_getenv(const char* szKey)
  {
    char* value = NULL;

    {
      CSingleLock lock(dll_cs_environ);

      update_emu_environ();//apply any changes

      for (int i = 0; i < EMU_MAX_ENVIRONMENT_ITEMS && value == NULL; i++)
      {
        if (dll__environ[i])
        {
          if (strnicmp(dll__environ[i], szKey, strlen(szKey)) == 0)
          {
            // found it
            value = dll__environ[i] + strlen(szKey) + 1;
          }
        }
      }
    }

    if (value != NULL)
    {
      return value;
    }

    return NULL;
  }

  int dll_ctype(int i)
  {
    not_implement("msvcrt.dll fake function dll_ctype() called\n");
    return 0;
  }

  int dll_system(const char *command)
  {
    not_implement("msvcrt.dll fake function dll_system() called\n");
    return 0; //system(command);
  }

  void (__cdecl * dll_signal(int sig, void (__cdecl *func)(int)))(int)
  {
#if defined(TARGET_WINDOWS)
    //vs2008 asserts for known signals, return err for everything unknown to windows.
    if (sig == 5 || sig == 7 || sig == 9 || sig == 10 || sig == 12 || sig == 14 || sig == 18 || sig == 19 || sig == 20)
      return SIG_ERR;
#endif
    return signal(sig, func);
  }

  int dll_getpid()
  {
    return 1;
  }

  int dll__commit(int fd)
  {
    CFile* pFile = g_emuFileWrapper.GetFileXbmcByDescriptor(fd);
    if (pFile != NULL)
    {
      pFile->Flush();
      return 0;
    }
    else if (!IS_STD_DESCRIPTOR(fd))
    {
      // it might be something else than a file, or the file is not emulated
      // let the operating system handle it
#ifndef TARGET_POSIX
      return _commit(fd);
#else
      return fsync(fd);
#endif
    }

    // std stream, no need to flush
    return 0;
  }

  char*** dll___p__environ()
  {
    static char*** t = (char***)&dll__environ;
    return (char***)&t;
  }

#ifdef TARGET_POSIX
#if defined(TARGET_ANDROID)
  volatile int * __cdecl dll_errno(void)
  {
    return &errno;
  }
#else
  int * __cdecl dll_errno(void)
  {
    return &errno;
  }
#endif

  int __cdecl dll_ioctl(int fd, unsigned long int request, va_list va)
  {
     int ret;
     CFile* pFile = g_emuFileWrapper.GetFileXbmcByDescriptor(fd);
     if (!pFile)
       return -1;

#if defined(HAS_DVD_DRIVE) && !defined(TARGET_FREEBSD)
#if !defined(TARGET_DARWIN)
    if(request == DVD_READ_STRUCT || request == DVD_AUTH)
#else
    if(request == DKIOCDVDSENDKEY || request == DKIOCDVDREPORTKEY || request == DKIOCDVDREADSTRUCTURE)
#endif
    {
      void *p1 = va_arg(va, void*);
      SNativeIoControl d;
      d.request = request;
      d.param   = p1;
      ret = pFile->IoControl(IOCTRL_NATIVE, &d);
      if(ret<0)
        CLog::Log(LOGWARNING, "%s - %ld request failed with error [%d] %s", __FUNCTION__, request, errno, strerror(errno));
    }
    else
#endif
    {
      CLog::Log(LOGWARNING, "%s - Unknown request type %ld", __FUNCTION__, request);
      ret = -1;
    }
    return ret;
  }
#endif

  int dll_setvbuf(FILE *stream, char *buf, int type, size_t size)
  {
    CLog::Log(LOGWARNING, "%s - May not be implemented correctly",
              __FUNCTION__);
    return 0;
  }

  struct mntent *dll_getmntent(FILE *fp)
  {
    if (fp == NULL)
      return NULL;

    CFile* pFile = g_emuFileWrapper.GetFileXbmcByStream(fp);
    if (pFile)
    {
      CLog::Log(LOGERROR, "%s - getmntent is not implemented for our virtual filesystem", __FUNCTION__);
      return NULL;
    }
#if defined(TARGET_LINUX)
    return getmntent(fp);
#else
    CLog::Log(LOGWARNING, "%s - unimplemented function called", __FUNCTION__);
    return NULL;
#endif
  }

  int dll_filbuf(FILE *fp)
  {
    if (fp == NULL)
      return EOF;

    if(IS_STD_STREAM(fp))
      return EOF;

    CFile* pFile = g_emuFileWrapper.GetFileXbmcByStream(fp);
    if (pFile)
    {
      unsigned char data;
      if(pFile->Read(&data, 1) == 1)
      {
        pFile->Seek(-1, SEEK_CUR);
        return (int)data;
      }
      else
        return EOF;
    }
#ifdef TARGET_POSIX
    return EOF;
#else
    return _filbuf(fp);
#endif
  }

  int dll_flsbuf(int data, FILE *fp)
  {
    if (fp == NULL)
      return EOF;

    if(IS_STDERR_STREAM(fp) || IS_STDOUT_STREAM(fp))
    {
      CLog::Log(LOGDEBUG, "dll_flsbuf() - %c", data);
      return data;
    }

    if(IS_STD_STREAM(fp))
      return EOF;

    CFile* pFile = g_emuFileWrapper.GetFileXbmcByStream(fp);
    if (pFile)
    {
      pFile->Flush();
      unsigned char c = (unsigned char)data;
      if(pFile->Write(&c, 1) == 1)
        return data;
      else
        return EOF;
    }
#ifdef TARGET_POSIX
    return EOF;
#else
    return _flsbuf(data, fp);
#endif
  }

  // this needs to be wrapped, since dll's have their own file
  // descriptor list, but we always use app's list with our wrappers
  int __cdecl dll_open_osfhandle(intptr_t _OSFileHandle, int _Flags)
  {
#ifdef TARGET_WINDOWS
    return _open_osfhandle(_OSFileHandle, _Flags);
#else
    return -1;
#endif
  }

}
