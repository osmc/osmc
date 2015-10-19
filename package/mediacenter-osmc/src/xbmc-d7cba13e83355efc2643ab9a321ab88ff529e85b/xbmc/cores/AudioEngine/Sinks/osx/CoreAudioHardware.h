#pragma once
/*
 *      Copyright (C) 2011-2013 Team XBMC
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

#include "system.h"

#if defined(TARGET_DARWIN_OSX)

#include "cores/AudioEngine/Sinks/osx/CoreAudioDevice.h"

// There is only one AudioSystemObject instance system-side.
// Therefore, all CCoreAudioHardware methods are static
class CCoreAudioHardware
{
public:
  static bool           GetAutoHogMode();
  static void           SetAutoHogMode(bool enable);
  static AudioStreamBasicDescription* FormatsList(AudioStreamID stream);
  static AudioStreamID* StreamsList(AudioDeviceID device);
  static void           ResetAudioDevices();
  static void           ResetStream(AudioStreamID streamId);
  static AudioDeviceID  FindAudioDevice(const std::string &deviceName);
  static AudioDeviceID  GetDefaultOutputDevice();
  static void           GetOutputDeviceName(std::string &name);
  static UInt32         GetOutputDevices(CoreAudioDeviceList *pList);
};

#endif
