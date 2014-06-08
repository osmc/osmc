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

#define HAS_ADL_EDID_PARSER

#include "lib/platform/os.h"
#include "lib/platform/util/edid.h"

#if !defined(__WINDOWS__)
#include "adl_sdk.h"
#include <dlfcn.h>
#include <stdlib.h>	
#include <string.h>
#include <unistd.h>

typedef void* ADL_LIB_HANDLE;

#else
#include <windows.h>
#include <tchar.h>
#include "adl_sdk.h"

typedef HINSTANCE ADL_LIB_HANDLE;
#endif

typedef int (*ADL_MAIN_CONTROL_CREATE )         (ADL_MAIN_MALLOC_CALLBACK, int);
typedef int (*ADL_MAIN_CONTROL_DESTROY)         (void);
typedef int (*ADL_ADAPTER_NUMBEROFADAPTERS_GET) (int*);
typedef int (*ADL_ADAPTER_ADAPTERINFO_GET)      (LPAdapterInfo, int);
typedef int (*ADL_DISPLAY_DISPLAYINFO_GET)      (int, int *, ADLDisplayInfo **, int);
typedef int (*ADL_DISPLAY_EDIDDATA_GET)         (int, int, ADLDisplayEDIDData *);

#define ADL_DISPLAY_CONNECTED (ADL_DISPLAY_DISPLAYINFO_DISPLAYCONNECTED | ADL_DISPLAY_DISPLAYINFO_DISPLAYMAPPED)

namespace PLATFORM
{
  class CADLEdidParser
  {
  public:
    CADLEdidParser(void);
    virtual ~CADLEdidParser(void);

    uint16_t GetPhysicalAddress(void);
    int GetNumAdapters(void);

  private:
    bool LibOpen(void) { return m_bOpen; }
    void Initialise(void);
    bool OpenLibrary(void);
    void CloseLibrary(void);

    LPAdapterInfo GetAdapterInfo(int iNumAdapters);
    bool GetAdapterEDID(int iAdapterIndex, int iDisplayIndex, ADLDisplayEDIDData *data);

    bool           m_bOpen;
    ADL_LIB_HANDLE m_handle;

    ADL_MAIN_CONTROL_CREATE          ADL_Main_Control_Create;
    ADL_MAIN_CONTROL_DESTROY         ADL_Main_Control_Destroy;
	  ADL_ADAPTER_NUMBEROFADAPTERS_GET ADL_Adapter_NumberOfAdapters_Get;
	  ADL_ADAPTER_ADAPTERINFO_GET      ADL_Adapter_AdapterInfo_Get;
	  ADL_DISPLAY_DISPLAYINFO_GET      ADL_Display_DisplayInfo_Get;
    ADL_DISPLAY_EDIDDATA_GET         ADL_Display_EdidData_Get;
  };
}
