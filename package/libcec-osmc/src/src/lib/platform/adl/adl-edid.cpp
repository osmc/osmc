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

#include "env.h"
#include "adl-edid.h"

// for dlsym and friends
#if defined(__WINDOWS__)
#include "../windows/dlfcn-win32.h"
#endif

using namespace PLATFORM;

CADLEdidParser::CADLEdidParser(void) :
  m_bOpen(false),
  m_handle(NULL)
{
  Initialise();
}

CADLEdidParser::~CADLEdidParser(void)
{
  CloseLibrary();
}

bool CADLEdidParser::OpenLibrary(void)
{
  CloseLibrary();

#if !defined(__WINDOWS__)
  m_handle = dlopen("libatiadlxx.so", RTLD_LAZY|RTLD_GLOBAL);
#else
  m_handle = LoadLibrary("atiadlxx.dll");
  // try 32 bit
  if (!m_handle)
    m_handle = LoadLibrary("atiadlxy.dll");
#endif

  return m_handle != NULL;
}

void CADLEdidParser::CloseLibrary(void)
{
  if (LibOpen())
    ADL_Main_Control_Destroy();

  if (m_handle)
    dlclose(m_handle);
  m_handle = NULL;
}

void *__stdcall ADL_AllocMemory(int iSize)
{
  void* lpBuffer = malloc(iSize);
  return lpBuffer;
}

void CADLEdidParser::Initialise(void)
{
  if (OpenLibrary())
  {
    // dlsym the methods we need
    ADL_Main_Control_Create          = (ADL_MAIN_CONTROL_CREATE)          dlsym(m_handle, "ADL_Main_Control_Create");
	  ADL_Main_Control_Destroy         = (ADL_MAIN_CONTROL_DESTROY)         dlsym(m_handle, "ADL_Main_Control_Destroy");
	  ADL_Adapter_NumberOfAdapters_Get = (ADL_ADAPTER_NUMBEROFADAPTERS_GET) dlsym(m_handle, "ADL_Adapter_NumberOfAdapters_Get");
	  ADL_Adapter_AdapterInfo_Get      = (ADL_ADAPTER_ADAPTERINFO_GET)      dlsym(m_handle, "ADL_Adapter_AdapterInfo_Get");
	  ADL_Display_DisplayInfo_Get      = (ADL_DISPLAY_DISPLAYINFO_GET)      dlsym(m_handle, "ADL_Display_DisplayInfo_Get");
	  ADL_Display_EdidData_Get         = (ADL_DISPLAY_EDIDDATA_GET)         dlsym(m_handle, "ADL_Display_EdidData_Get");

	  // check whether they could all be resolved
    if (ADL_Main_Control_Create &&
        ADL_Main_Control_Destroy &&
        ADL_Adapter_NumberOfAdapters_Get &&
        ADL_Adapter_AdapterInfo_Get &&
        ADL_Display_DisplayInfo_Get &&
        ADL_Display_EdidData_Get)
    {
      // and try to initialise it
      m_bOpen = (ADL_OK == ADL_Main_Control_Create(ADL_AllocMemory, 1));
    }
  }
}

int CADLEdidParser::GetNumAdapters(void)
{
  int iNumAdapters(0);

  if (!LibOpen() || ADL_OK != ADL_Adapter_NumberOfAdapters_Get(&iNumAdapters))
    iNumAdapters = 0;

  return iNumAdapters;
}

LPAdapterInfo CADLEdidParser::GetAdapterInfo(int iNumAdapters)
{
  // validate input
  if (iNumAdapters <= 0)
    return NULL;

  LPAdapterInfo adapterInfo = (LPAdapterInfo)malloc(sizeof(AdapterInfo) * iNumAdapters);
  memset(adapterInfo, 0, sizeof(AdapterInfo) * iNumAdapters);

  // get the info
  ADL_Adapter_AdapterInfo_Get(adapterInfo, sizeof(AdapterInfo) * iNumAdapters);

  return adapterInfo;
}

bool CADLEdidParser::GetAdapterEDID(int iAdapterIndex, int iDisplayIndex, ADLDisplayEDIDData *data)
{
  // validate input
  if (iAdapterIndex < 0 || iDisplayIndex < 0)
    return false;

  memset(data, 0, sizeof(ADLDisplayEDIDData));
  data->iSize = sizeof(ADLDisplayEDIDData);
  data->iBlockIndex = 1;

  return (ADL_Display_EdidData_Get(iAdapterIndex, iDisplayIndex, data) == ADL_OK);
}

uint16_t CADLEdidParser::GetPhysicalAddress(void)
{
  uint16_t iPA(0);

  // get the number of adapters
  int iNumAdapters = GetNumAdapters();
  if (iNumAdapters <= 0)
    return 0;

  // get the adapter info
  LPAdapterInfo adapterInfo = GetAdapterInfo(iNumAdapters);
  if (!adapterInfo)
    return 0;

  // iterate over it
  for (int iAdapterPtr = 0; iAdapterPtr < iNumAdapters; iAdapterPtr++)
  {
    int iNumDisplays(-1);
    LPADLDisplayInfo displayInfo(NULL);
    int iAdapterIndex = adapterInfo[iAdapterPtr].iAdapterIndex;

    // get the display info
    if (ADL_OK != ADL_Display_DisplayInfo_Get(iAdapterIndex, &iNumDisplays, &displayInfo, 0))
      continue;

    // iterate over it
    for (int iDisplayPtr = 0; iDisplayPtr < iNumDisplays; iDisplayPtr++)
		{
      // check whether the display is connected
      if ((displayInfo[iDisplayPtr].iDisplayInfoValue & ADL_DISPLAY_CONNECTED) != ADL_DISPLAY_CONNECTED)
        continue;

      int iDisplayIndex = displayInfo[iDisplayPtr].displayID.iDisplayLogicalIndex;

      // try to get the EDID
      ADLDisplayEDIDData edidData;
      if (GetAdapterEDID(iAdapterIndex, iDisplayIndex, &edidData))
			{
        // try to get the PA from the EDID
        iPA = CEDIDParser::GetPhysicalAddressFromEDID(edidData.cEDIDData, edidData.iEDIDSize);

        // found it
        if (iPA != 0)
          break;
			}
		}

    free(displayInfo);
  }

  free(adapterInfo);

  return iPA;
}
