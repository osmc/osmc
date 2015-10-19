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
// VideoSettings.cpp: implementation of the CVideoSettings class.
//
//////////////////////////////////////////////////////////////////////

#include "VideoSettings.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CVideoSettings::CVideoSettings()
{
  m_DeinterlaceMode = VS_DEINTERLACEMODE_AUTO;
  m_InterlaceMethod = VS_INTERLACEMETHOD_AUTO;
  m_ScalingMethod = VS_SCALINGMETHOD_LINEAR;
  m_ViewMode = ViewModeNormal;
  m_CustomZoomAmount = 1.0f;
  m_CustomPixelRatio = 1.0f;
  m_CustomVerticalShift = 0.0f;
  m_CustomNonLinStretch = false;
  m_AudioStream = -1;
  m_SubtitleStream = -1;
  m_SubtitleDelay = 0.0f;
  m_SubtitleOn = true;
  m_SubtitleCached = false;
  m_Brightness = 50.0f;
  m_Contrast = 50.0f;
  m_Gamma = 20.0f;
  m_Sharpness = 0.0f;
  m_NoiseReduction = 0;
  m_PostProcess = false;
  m_VolumeAmplification = 0;
  m_AudioDelay = 0.0f;
  m_OutputToAllSpeakers = false;
  m_ResumeTime = 0;
  m_StereoMode = 0;
  m_StereoInvert = false;

}

bool CVideoSettings::operator!=(const CVideoSettings &right) const
{
  if (m_DeinterlaceMode != right.m_DeinterlaceMode) return true;
  if (m_InterlaceMethod != right.m_InterlaceMethod) return true;
  if (m_ScalingMethod != right.m_ScalingMethod) return true;
  if (m_ViewMode != right.m_ViewMode) return true;
  if (m_CustomZoomAmount != right.m_CustomZoomAmount) return true;
  if (m_CustomPixelRatio != right.m_CustomPixelRatio) return true;
  if (m_CustomVerticalShift != right.m_CustomVerticalShift) return true;
  if (m_CustomNonLinStretch != right.m_CustomNonLinStretch) return true;
  if (m_AudioStream != right.m_AudioStream) return true;
  if (m_SubtitleStream != right.m_SubtitleStream) return true;
  if (m_SubtitleDelay != right.m_SubtitleDelay) return true;
  if (m_SubtitleOn != right.m_SubtitleOn) return true;
  if (m_SubtitleCached != right.m_SubtitleCached) return true;
  if (m_Brightness != right.m_Brightness) return true;
  if (m_Contrast != right.m_Contrast) return true;
  if (m_Gamma != right.m_Gamma) return true;
  if (m_Sharpness != right.m_Sharpness) return true;
  if (m_NoiseReduction != right.m_NoiseReduction) return true;
  if (m_PostProcess != right.m_PostProcess) return true;
  if (m_VolumeAmplification != right.m_VolumeAmplification) return true;
  if (m_AudioDelay != right.m_AudioDelay) return true;
  if (m_OutputToAllSpeakers != right.m_OutputToAllSpeakers) return true;
  if (m_ResumeTime != right.m_ResumeTime) return true;
  if (m_StereoMode != right.m_StereoMode) return true;
  if (m_StereoInvert != right.m_StereoInvert) return true;
  return false;
}
