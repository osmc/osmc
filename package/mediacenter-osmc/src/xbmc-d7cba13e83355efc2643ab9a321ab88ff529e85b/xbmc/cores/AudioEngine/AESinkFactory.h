#pragma once
/*
 *      Copyright (C) 2010-2013 Team XBMC
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

#include <stdint.h>
#include <string>
#include <vector>

#include "cores/AudioEngine/Utils/AEDeviceInfo.h"

class IAESink;

typedef struct
{
  std::string      m_sinkName;
  AEDeviceInfoList m_deviceInfoList;
} AESinkInfo;

typedef std::vector<AESinkInfo> AESinkInfoList;

class CAESinkFactory
{
public:
  static void     ParseDevice(std::string &device, std::string &driver);
  static IAESink *Create(std::string &device, AEAudioFormat &desiredFormat, bool rawPassthrough);
  static void     EnumerateEx(AESinkInfoList &list, bool force = false); /* The force flag can be used to indicate the rescan devices */

protected:
  static IAESink *TrySink(std::string &driver, std::string &device, AEAudioFormat &format);
};

