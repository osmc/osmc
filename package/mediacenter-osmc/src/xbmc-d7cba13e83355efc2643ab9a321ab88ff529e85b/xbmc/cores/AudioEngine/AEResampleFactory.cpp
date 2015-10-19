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

#include "AEResampleFactory.h"
#include "cores/AudioEngine/Engines/ActiveAE/ActiveAEResampleFFMPEG.h"
#if defined(TARGET_RASPBERRY_PI)
  #include "settings/Settings.h"
  #include "cores/AudioEngine/Engines/ActiveAE/ActiveAEResamplePi.h"
#endif

namespace ActiveAE
{

IAEResample *CAEResampleFactory::Create(uint32_t flags /* = 0 */)
{
#if defined(TARGET_RASPBERRY_PI)
  if (!(flags & AERESAMPLEFACTORY_QUICK_RESAMPLE) && CSettings::GetInstance().GetInt(CSettings::SETTING_AUDIOOUTPUT_PROCESSQUALITY) == AE_QUALITY_GPU)
    return new CActiveAEResamplePi();
#endif
  return new CActiveAEResampleFFMPEG();
}

}
