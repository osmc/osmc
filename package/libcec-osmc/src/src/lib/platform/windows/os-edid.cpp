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
#include "lib/platform/util/edid.h"

#include "windows.h"
#include "setupapi.h"
#include "initguid.h"
#include "stdio.h"

using namespace PLATFORM;

static GUID MONITOR_GUID =  { 0x4D36E96E, 0xE325, 0x11CE, { 0xBF, 0xC1, 0x08, 0x00, 0x2B, 0xE1, 0x03, 0x18 } };
#define PA_MAX_REGENTRIES_TO_CHECK 1024

uint16_t GetPhysicalAddressFromDevice(IN HDEVINFO hDevHandle, IN PSP_DEVINFO_DATA deviceInfoData)
{
  uint16_t iPA(0);

  HKEY hDevRegKey = SetupDiOpenDevRegKey(hDevHandle, deviceInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_ALL_ACCESS);
  if (hDevRegKey)
  {
    CHAR  regEntryName[128];
    DWORD regEntryNameLength(128);
    DWORD type;
    LONG  retVal(ERROR_SUCCESS);

    for (LONG ptr = 0; iPA == 0 && retVal == ERROR_SUCCESS && ptr < PA_MAX_REGENTRIES_TO_CHECK; ptr++)
    {
      BYTE regEntryData[1024];
      DWORD regEntryDataSize = sizeof(regEntryData);

      retVal = RegEnumValue(hDevRegKey, ptr, &regEntryName[0], &regEntryNameLength, NULL, &type, regEntryData, &regEntryDataSize);

      if (retVal == ERROR_SUCCESS && !strcmp(regEntryName,"EDID"))
        iPA = CEDIDParser::GetPhysicalAddressFromEDID(regEntryData, regEntryDataSize);
    }
    RegCloseKey(hDevRegKey);
  }

  return iPA;
}

uint16_t CEDIDParser::GetPhysicalAddress(void)
{
  HDEVINFO hDevHandle;
  uint16_t iPA(0);
  SP_DEVINFO_DATA deviceInfoData;

  hDevHandle = SetupDiGetClassDevsEx(&MONITOR_GUID, NULL, NULL, DIGCF_PRESENT, NULL,  NULL, NULL);
  if (!hDevHandle)
    return iPA;

  for (int i=0; ERROR_NO_MORE_ITEMS != GetLastError(); i++)
  {
    memset(&deviceInfoData, 0, sizeof(SP_DEVINFO_DATA));
    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    if (SetupDiEnumDeviceInfo(hDevHandle,i, &deviceInfoData))
    {
      iPA = GetPhysicalAddressFromDevice(hDevHandle, &deviceInfoData);
    }
  }

  return iPA;
}
