#pragma once
/*
 * This file is part of the libCEC(R) library.
 *
 * libCEC(R) is Copyright (C) 2011-2013 Pulse-Eight Limited.  All rights reserved.
 * libCEC(R) is an original work, containing original code.
 *
 * libCEC(R) is a trademark of Pulse-Eight Limited.
 *
 * This program is dual-licensed; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * Alternatively, you can license this library under a commercial license,
 * please contact Pulse-Eight Licensing for more information.
 *
 * For more information contact:
 * Pulse-Eight Licensing       <license@pulse-eight.com>
 *     http://www.pulse-eight.com/
 *     http://www.pulse-eight.net/
 */

#ifndef CECLOADER_H_
#define CECLOADER_H_

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <conio.h>

HINSTANCE g_libCEC = NULL;

/*!
 * @brief Create a new libCEC instance.
 * @param configuration The configuration to pass to libCEC
 * @param strLib The name of and/or path to libCEC
 * @return An instance of ICECAdapter or NULL on error.
 */
CEC::ICECAdapter *LibCecInitialise(CEC::libcec_configuration *configuration, const char *strLib = NULL)
{
  if (!g_libCEC)
#if defined(_WIN64)
    g_libCEC = LoadLibrary(strLib ? strLib : "libcec.x64.dll");
#else
    g_libCEC = LoadLibrary(strLib ? strLib : "libcec.dll");
#endif
  if (!g_libCEC)
    return NULL;

  typedef void* (__cdecl*_LibCecInitialise)(CEC::libcec_configuration *);
  _LibCecInitialise LibCecInitialise;
  LibCecInitialise = (_LibCecInitialise) (GetProcAddress(g_libCEC, "CECInitialise"));
  if (!LibCecInitialise)
  {
    cout << "cannot find CECInitialise" << endl;
    return NULL;
  }

  return static_cast< CEC::ICECAdapter* > (LibCecInitialise(configuration));
}

/*!
 * @brief Destroy an instance of libCEC.
 * @param device The instance to destroy.
 */
void UnloadLibCec(CEC::ICECAdapter *device)
{
  typedef void (__cdecl*_DestroyLibCec)(void * device);
  _DestroyLibCec DestroyLibCec;
  DestroyLibCec = (_DestroyLibCec) (GetProcAddress(g_libCEC, "CECDestroy"));
  if (DestroyLibCec)
    DestroyLibCec(device);

  FreeLibrary(g_libCEC);
  g_libCEC = NULL;
}

/*!
 * @brief Start the bootloader on the first device that was detected.
 * @param strLib The name of and/or path to libCEC
 * @return True when the command was sent, false otherwise.
 */
bool LibCecBootloader(const char *strLib = NULL)
{
  if (!g_libCEC)
#if defined(_WIN64)
    g_libCEC = LoadLibrary(strLib ? strLib : "libcec.x64.dll");
#else
    g_libCEC = LoadLibrary(strLib ? strLib : "libcec.dll");
#endif
  if (!g_libCEC)
    return NULL;

  typedef bool (__cdecl*_LibCecBootloader)(void);
  _LibCecBootloader LibCecBootloader;
  LibCecBootloader = (_LibCecBootloader) (GetProcAddress(g_libCEC, "CECStartBootloader"));
  if (!LibCecBootloader)
    return false;

  bool bReturn = LibCecBootloader();
  FreeLibrary(g_libCEC);
  g_libCEC = NULL;
  return bReturn;
}

#else

#include <dlfcn.h>

void *g_libCEC = NULL;

/*!
 * @brief Create a new libCEC instance.
 * @param configuration The configuration to pass to libCEC
 * @param strLib The name of and/or path to libCEC
 * @return An instance of ICECAdapter or NULL on error.
 */
CEC::ICECAdapter *LibCecInitialise(CEC::libcec_configuration *configuration, const char *strLib = NULL)
{
  if (!g_libCEC)
  {
#if defined(__APPLE__)
    g_libCEC = dlopen(strLib ? strLib : "libcec." CEC_LIB_VERSION_MAJOR_STR ".dylib", RTLD_LAZY);
#else
    g_libCEC = dlopen(strLib ? strLib : "libcec.so." CEC_LIB_VERSION_MAJOR_STR, RTLD_LAZY);
#endif
    if (!g_libCEC)
    {
      cout << dlerror() << endl;
      return NULL;
    }
  }

  typedef void* _LibCecInitialise(CEC::libcec_configuration *);
  _LibCecInitialise* LibCecInitialise = (_LibCecInitialise*) dlsym(g_libCEC, "CECInitialise");
  if (!LibCecInitialise)
  {
    cout << "cannot find CECInitialise" << endl;
    return NULL;
  }

  return (CEC::ICECAdapter*) LibCecInitialise(configuration);
}

/*!
 * @brief Destroy an instance of libCEC.
 * @param device The instance to destroy.
 */
void UnloadLibCec(CEC::ICECAdapter *device)
{
  typedef void* _DestroyLibCec(CEC::ICECAdapter *);
  _DestroyLibCec *DestroyLibCec = (_DestroyLibCec*) dlsym(g_libCEC, "CECDestroy");
  if (DestroyLibCec)
    DestroyLibCec(device);

  dlclose(g_libCEC);
}

/*!
 * @brief Start the bootloader on the first device that was detected.
 * @param strLib The name of and/or path to libCEC
 * @return True when the command was sent, false otherwise.
 */
bool LibCecBootloader(const char *strLib = NULL)
{
  if (!g_libCEC)
  {
#if defined(__APPLE__)
    g_libCEC = dlopen(strLib ? strLib : "libcec.dylib", RTLD_LAZY);
#else
    g_libCEC = dlopen(strLib ? strLib : "libcec.so." CEC_LIB_VERSION_MAJOR_STR, RTLD_LAZY);
#endif
    if (!g_libCEC)
    {
      cout << dlerror() << endl;
      return NULL;
    }
  }

  typedef bool _LibCecBootloader(void);
  _LibCecBootloader* LibCecBootloader = (_LibCecBootloader*) dlsym(g_libCEC, "CECStartBootloader");
  if (!LibCecBootloader)
  {
    cout << "cannot find CECStartBootloader" << endl;
    return NULL;
  }

  bool bReturn = LibCecBootloader();
  dlclose(g_libCEC);
  return bReturn;
}

#endif

#endif /* CECLOADER_H_ */
